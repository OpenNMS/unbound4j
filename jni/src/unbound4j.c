/*
 * Copyright 2019, The OpenNMS Group
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <pthread.h>
#include <stdio.h>
#include <unbound.h>
#include <sys/shm.h>

#include "uthash.h"
#include "unbound4j.h"
#include "sldns.h"
#include "dnsutils.h"

struct ub4j_context *contexts = NULL;
pthread_rwlock_t g_ctx_lock;
pthread_mutex_t g_cfg_lock;

void ub4j_init() {
    if (pthread_rwlock_init(&g_ctx_lock, NULL) != 0) {
        printf("unbound4j: Error while initializing read-write lock.\n");
    }

    if (pthread_mutex_init(&g_cfg_lock, NULL) != 0) {
        printf("unbound4j: Error while initializing configuration lock.\n");
    }
}

void ub4j_destroy() {
    char error[256];
    size_t error_len = sizeof(error);

    // Delete all outstanding contexts
    struct ub4j_context *ctx, *tmp;
    HASH_ITER(hh, contexts, ctx, tmp) {
        if(ub4j_delete_context(ctx->id, error, error_len)) {
            printf("unbound4j: Deleting context failed: %s", error);
        }
    }
}

void* context_processing_thread(void *arg);

void ub4j_print_stats(long ctx_id) {
    struct ub_stats_info* stats;
    struct ub_shm_stat_info* shm_stat;
    int id_ctl, id_arr;

    // get shm segments
    id_ctl = shmget(11777, sizeof(int), SHM_R|SHM_W);
    if(id_ctl == -1) {
        printf("shmget failed!");
        return;
    }
    id_arr = shmget(11777+1, sizeof(int), SHM_R|SHM_W);
    if(id_arr == -1) {
        printf("shmget failed!!");
        return;
    }
    shm_stat = (struct ub_shm_stat_info*)shmat(id_ctl, NULL, 0);
    if(shm_stat == (void*)-1) {
        printf("shmat failed!");
        return;
    }
    stats = (struct ub_stats_info*)shmat(id_arr, NULL, 0);
    if(stats == (void*)-1) {
        printf("shmat failed!!");
        return;
    }

    shmdt(shm_stat);
    shmdt(stats);
}

struct ub4j_context* ub4j_create_context(struct ub4j_config* config, char* error, size_t error_len) {
    int retval;
    struct ub4j_context *ctx = malloc(sizeof(struct ub4j_context));
    if (ctx == NULL) {
        snprintf(error, error_len, "Failed to allocate memory for context.");
        return NULL;
    }
    memset(ctx, 0, sizeof(struct ub4j_context));

    ctx->ub_ctx = ub_ctx_create();
    if(!ctx->ub_ctx) {
        free(ctx);
        snprintf(error, error_len, "Could not create Unbound context.");
        return NULL;
    }

    if (config->use_system_resolver) {
        // Read /etc/resolv.conf for DNS proxy settings
        if( (retval=ub_ctx_resolvconf(ctx->ub_ctx, "/etc/resolv.conf")) != 0) {
            snprintf(error, error_len, "Error reading resolv.conf: %s", ub_strerror(retval));
            ub_ctx_delete(ctx->ub_ctx);
            return NULL;
        }

        // Read /etc/hosts for locally supplied host addresses
        if( (retval=ub_ctx_hosts(ctx->ub_ctx, "/etc/hosts")) != 0) {
            snprintf(error, error_len, "Error reading hosts: %s", ub_strerror(retval));
            return NULL;
        }
    } else if (config->unbound_config != NULL) {
        // Loading the configuration this way is not thread safe, so let's make sure we're only loading one at a time
        pthread_mutex_lock(&g_cfg_lock);
        if( (retval=ub_ctx_config(ctx->ub_ctx, config->unbound_config)) != 0) {
            pthread_mutex_unlock(&g_cfg_lock);
            snprintf(error, error_len, "Error reading Unbound configuration from '%s': %s", config->unbound_config, ub_strerror(retval));
            ub_ctx_delete(ctx->ub_ctx);
            return NULL;
        }
        pthread_mutex_unlock(&g_cfg_lock);
    }

    /* Enable statistics?
    ub_ctx_set_option(ctx->ub_ctx, "statistics-interval:", "1");
    ub_ctx_set_option(ctx->ub_ctx, "shm-enable:", "yes");
    ub_ctx_set_option(ctx->ub_ctx, "shm-key:", "11777");
    */

    ctx->ub_fd = ub_fd(ctx->ub_ctx);
    if (ctx->ub_fd < 0) {
        snprintf(error, error_len, "Failed to acquire file description from Unbound context.");
        ub_ctx_delete(ctx->ub_ctx);
        return NULL;
    }

    // Spawn the thread
    if (pthread_create(&(ctx->thread_id), NULL, context_processing_thread, ctx)) {
        snprintf(error, error_len, "Failed to create processing thread for context.");
        ub_ctx_delete(ctx->ub_ctx);
        return NULL;
    }

    // Use the thread id as the context id, since this will be unique
    ctx->id = ctx->thread_id;

    // Store the context
    if (pthread_rwlock_wrlock(&g_ctx_lock) != 0) {
        snprintf(error, error_len, "Failed to acquire write lock.");
        ctx->stopping = 1; // Stop the thread
        ub_ctx_delete(ctx->ub_ctx);
        return NULL;
    }
    HASH_ADD_INT(contexts, id, ctx);
    pthread_rwlock_unlock(&g_ctx_lock);
    return ctx;
}

int ub4j_delete_context(long ctx_id, char* error, size_t error_len) {
    // Acquire a write lock
    if (pthread_rwlock_wrlock(&g_ctx_lock) != 0) {
        snprintf(error, error_len, "Failed to acquire write lock.");
        return -1;
    }

    // Lookup the context by id and remove it from the hashmap if found
    long id = (long)ctx_id;
    struct ub4j_context *ctx = NULL;
    HASH_FIND_INT(contexts, &id, ctx);
    if (ctx != NULL) {
        HASH_DEL(contexts, ctx);
    }

    // Release the lock
    pthread_rwlock_unlock(&g_ctx_lock);

    // Nothing to do if there was no context found
    if (ctx == NULL) {
        return 0;
    }

    int nret = 0;

    // Stop the thread and join
    ctx->stopping = 1;
    if (pthread_join(ctx->thread_id, NULL)) {
        snprintf(error, error_len, "Error on join for processing thread.");
        nret = -1;
    }

    // Delete the Unbound context
    ub_ctx_delete(ctx->ub_ctx);

    // Free up the ub4j context structure
    free(ctx);
    return nret;
}

typedef struct {
    void* userdata;
    ub4j_callback_type callback;
} ub4j_callback_context;

void ub_reverse_lookup_callback(void* mydata, int err, struct ub_result* result) {
    ub4j_callback_context* callback_context = (ub4j_callback_context*)mydata;

    char* hostname = NULL;
    if (err == 0 && result != NULL) {
        if(result->havedata) {
            size_t hostname_len = 256; // maximum length of a domain name is 253
            hostname = malloc(hostname_len);
            sldns_wire2str_rdata_buf((uint8_t *) result->data[0], (size_t) result->len[0], hostname, hostname_len,
                                     (uint16_t) result->qtype);
        }
    }

    const char* err_str  = NULL;
    if (err != 0) {
        err_str = ub_strerror(err);
    }

    if (result != NULL) {
        ub_resolve_free(result);
    }

    // issue the delegate callback
    callback_context->callback(callback_context->userdata, err_str, hostname);
    free(callback_context);
}

int ub4j_reverse_lookup(long ctx_id, uint8_t* addr, size_t addr_len, void* userdata, ub4j_callback_type callback, char* error, size_t error_len) {
    // Acquire a read lock
    if (pthread_rwlock_rdlock(&g_ctx_lock) != 0) {
        snprintf(error, error_len, "Failed to acquire read lock.");
        return -1;
    }

    // Lookup the context by id
    long id = (long)ctx_id;
    struct ub4j_context *ctx = NULL;
    HASH_FIND_INT(contexts, &id, ctx);

    // Release the read lock
    pthread_rwlock_unlock(&g_ctx_lock);
    if (ctx == NULL) {
        snprintf(error, error_len, "Invalid context id.");
        return -1;
    }

    // Convert the IP address to a name used for reverse lookups i.e.:
    //  192.0.2.5 -> 5.2.0.192.in-addr.arpa.
    //  2001:db8::567:89ab -> b.a.9.8.7.6.5.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.8.b.d.0.1.0.0.2.ip6.arpa.
    char* reverse_lookup_domain;
    if (addr_len == 4) {
        build_reverse_lookup_domain_v4((struct in_addr*)addr, &reverse_lookup_domain);
    } else if (addr_len == 16) {
        build_reverse_lookup_domain_v6((struct in6_addr*)addr, &reverse_lookup_domain);
    } else {
        snprintf(error, error_len, "Invalid IP address length: %zu", addr_len);
        return -1;
    }

    if (reverse_lookup_domain == NULL) {
        snprintf(error, error_len, "Failed to allocate memory for reverse lookup domain.");
        return -1;
    }

    ub4j_callback_context* callback_context = malloc(sizeof(ub4j_callback_context));
    if (callback_context == NULL) {
        snprintf(error, error_len, "Failed to allocate memory for callback context.");
        free(reverse_lookup_domain);
        return -1;
    }

    memset(callback_context, 0, sizeof(ub4j_callback_context));
    callback_context->userdata = userdata;
    callback_context->callback = callback;

    // Issue the reverse lookup
    int nret = ub_resolve_async(ctx->ub_ctx, reverse_lookup_domain,
                              12 /* RR_TYPE_PTR */,
                              1 /* CLASS IN (internet) */,
                              callback_context,
                              ub_reverse_lookup_callback,
                              NULL);

    // We done with the domain name now
    free(reverse_lookup_domain);

    if (nret) {
        // Free the callback context
        free(callback_context);
        snprintf(error, error_len, "Resolve error: %s", ub_strerror(nret));
        return -1;
    }

    return 0;
}

void* context_processing_thread(void *arg) {
    struct ub4j_context *ctx = (struct ub4j_context *)arg;

    struct timeval tv;
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(ctx->ub_fd, &rfds);

    while(!ctx->stopping) {
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int ret = select(FD_SETSIZE, &rfds, NULL, NULL, &tv);
        if (ret >= 0) {
            ub_process(ctx->ub_ctx);
        }
    }

    return NULL;
}
