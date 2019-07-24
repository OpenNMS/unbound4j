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
#include <stdatomic.h>
#include <sys/time.h>

#include "uthash.h"
#include "unbound4j.h"
#include "sldns.h"
#include "dnsutils.h"

struct ub4j_query {
    int id;
    void* userdata;
    ub4j_callback_type callback;
    __time_t expires_at_epoch_sec;
    unsigned char expired;
    UT_hash_handle hh; // makes this structure hashable
};

struct ub4j_context *g_contexts = NULL;
atomic_int g_ctx_id_generator = ATOMIC_VAR_INIT(1);

struct ub4j_query *g_queries = NULL;

pthread_rwlock_t g_ctx_lock;
pthread_rwlock_t g_query_lock;
pthread_mutex_t g_cfg_lock;

void ub4j_init() {
    if (pthread_rwlock_init(&g_ctx_lock, NULL) != 0) {
        printf("unbound4j: Error while initializing read-write lock.\n");
    }

    if (pthread_rwlock_init(&g_query_lock, NULL) != 0) {
        printf("unbound4j: Error while initializing read-write lock.\n");
    }

    if (pthread_mutex_init(&g_cfg_lock, NULL) != 0) {
        printf("unbound4j: Error while initializing configuration lock.\n");
    }
}

int ub4j_free_context(struct ub4j_context *ctx, char* error, size_t error_len);

void ub4j_destroy() {
    char error[256];
    size_t error_len = sizeof(error);

    // Acquire a write lock
    if (pthread_rwlock_wrlock(&g_ctx_lock) != 0) {
        printf("unbound4j: Acquiring write lock failed.");
        return;
    }

    // Delete all outstanding contexts
    struct ub4j_context *ctx, *tmp;
    HASH_ITER(hh, g_contexts, ctx, tmp) {
        HASH_DEL(g_contexts, ctx);
        if(ub4j_free_context(ctx, error, error_len)) {
            printf("unbound4j: Deleting context failed: %s", error);
        }
    }

    // Release the lock
    pthread_rwlock_unlock(&g_ctx_lock);
}

void ub4j_config_init(struct ub4j_config* config) {
    config->request_timeout_secs = 5;
    config->use_system_resolver = 1;
    config->unbound_config = NULL;
}

void* context_processing_thread(void *arg);

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

    // Enable debugging
    // ub_ctx_debuglevel(ctx->ub_ctx, 3);

    // Use a thread instead of forking
    if (ub_ctx_async(ctx->ub_ctx, 1)) {
        snprintf(error, error_len, "Failed to configure asynchronous behaviour on Unbound context.");
        ub_ctx_delete(ctx->ub_ctx);
        return NULL;
    }

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

    // Generate a unique context id
    ctx->id = atomic_fetch_add(&g_ctx_id_generator, 1);

    // Store the configuration settings that we'll need later
    ctx->request_timeout_secs = config->request_timeout_secs;

    // Store the context
    if (pthread_rwlock_wrlock(&g_ctx_lock) != 0) {
        snprintf(error, error_len, "Failed to acquire write lock.");
        ctx->stopping = 1; // Stop the thread
        ub_ctx_delete(ctx->ub_ctx);
        return NULL;
    }

    HASH_ADD_INT(g_contexts, id, ctx);
    pthread_rwlock_unlock(&g_ctx_lock);
    return ctx;
}

int ub4j_delete_context(int ctx_id, char* error, size_t error_len) {
    // Acquire a write lock
    if (pthread_rwlock_wrlock(&g_ctx_lock) != 0) {
        snprintf(error, error_len, "Failed to acquire write lock.");
        return -1;
    }

    // Lookup the context by id and remove it from the hashmap if found
    int id = ctx_id;
    struct ub4j_context *ctx = NULL;
    HASH_FIND_INT(g_contexts, &id, ctx);
    if (ctx != NULL) {
        HASH_DEL(g_contexts, ctx);
    }

    // Release the lock
    pthread_rwlock_unlock(&g_ctx_lock);

    // Nothing to do if there was no context found
    if (ctx == NULL) {
        return 0;
    }

    return ub4j_free_context(ctx, error, error_len);
}

int ub4j_free_context(struct ub4j_context *ctx, char* error, size_t error_len) {
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

void ub_reverse_lookup_callback(void* mydata, int err, struct ub_result* result) {
    struct ub4j_query* query = (struct ub4j_query*)mydata;

    char* hostname = NULL;
    if (err == 0 && result != NULL) {
        if(result->havedata) {
            size_t hostname_len = 256; // maximum length of a domain name is 253
            hostname = malloc(hostname_len);
            // TODO: Catch malloc failure
            sldns_wire2str_rdata_buf((uint8_t *) result->data[0], (size_t) result->len[0], hostname, hostname_len,
                                     (uint16_t) result->qtype);
        }
    }

    const char* err_str  = NULL;
    if (err != 0) {
        if (query->expired) {
            err_str = "Query timed out.";
        } else {
            err_str = ub_strerror(err);
        }
    }

    if (result != NULL) {
        ub_resolve_free(result);
    }

    // Issue the delegate callback
    query->callback(query->userdata, err_str, hostname);

    // Stop tracking the query
    if (!query->expired) {
        if (!pthread_rwlock_wrlock(&g_query_lock)) {
            HASH_DEL(g_queries, query);
            pthread_rwlock_unlock(&g_query_lock);
        } else {
            printf("unbound4j: Failed to acquire write lock.");
        }
    }

    free(query);
}

int ub4j_reverse_lookup(int ctx_id, uint8_t* addr, size_t addr_len, void* userdata, ub4j_callback_type callback, char* error, size_t error_len) {
    // Acquire a read lock
    if (pthread_rwlock_rdlock(&g_ctx_lock) != 0) {
        snprintf(error, error_len, "Failed to acquire read lock.");
        return -1;
    }

    // Lookup the context by id
    int id = (int)ctx_id;
    struct ub4j_context *ctx = NULL;
    HASH_FIND_INT(g_contexts, &id, ctx);

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

    struct ub4j_query* query = malloc(sizeof(struct ub4j_query));
    if (query == NULL) {
        snprintf(error, error_len, "Failed to allocate memory for query context.");
        free(reverse_lookup_domain);
        return -1;
    }

    memset(query, 0, sizeof(struct ub4j_query));
    query->userdata = userdata;
    query->callback = callback;

    // Grab a write lock for the query tracking *before* we actually make the call
    if (pthread_rwlock_wrlock(&g_query_lock) != 0) {
        snprintf(error, error_len, "Failed to acquire write lock.");
        return -1;
    }

    // Set the expiry time
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    query->expires_at_epoch_sec = tv_now.tv_sec + ctx->request_timeout_secs;

    // Issue the reverse lookup
    int nret = ub_resolve_async(ctx->ub_ctx, reverse_lookup_domain,
                              12 /* RR_TYPE_PTR */,
                              1 /* CLASS IN (internet) */,
                              query,
                              ub_reverse_lookup_callback,
                              &query->id);

    // We're done with the domain name now
    free(reverse_lookup_domain);

    if (nret) {
        // The async query failed to be submitted, free the query context
        free(query);
        snprintf(error, error_len, "Resolve error: %s", ub_strerror(nret));
    } else {
        // The async query was successfully submitted, let's track it
        HASH_ADD_INT(g_queries, id, query);
    }

    // Release the write lock
    pthread_rwlock_unlock(&g_query_lock);

    return nret;
}

void* context_processing_thread(void *arg) {
    struct ub4j_context *ctx = (struct ub4j_context *)arg;


    struct ub4j_query *query, *query_tmp;

    struct timeval tv;
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(ctx->ub_fd, &rfds);

    while(!ctx->stopping) {
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int ret = select(FD_SETSIZE, &rfds, NULL, NULL, &tv);
        if (ret > 0) {
            if(ub_process(ctx->ub_ctx)) {
                printf("unbound4j: ub_process() error!\n");
            }
        }

        // Get the current time
        struct timeval tv_now;
        gettimeofday(&tv_now, NULL);

        // Acquire a read lock
        if (pthread_rwlock_rdlock(&g_query_lock) != 0) {
            printf("Failed to acquire read lock.");
            continue;
        }

        // Peek at the head of the list and determine whether or not we need to cancel any queries
        unsigned char need_to_cancel_queries = 0;
        query = g_queries;
        if (query != NULL && query->expires_at_epoch_sec <= tv_now.tv_sec) {
            need_to_cancel_queries = 1;
        }

        // Release our read lock
        pthread_rwlock_unlock(&g_query_lock);

        if (need_to_cancel_queries) {
            // Acquire a write lock
            if (pthread_rwlock_wrlock(&g_query_lock) != 0) {
                printf("unbound4j: Failed to acquire write lock.");
                continue;
            }

            // Iterate through the outstanding queries
            HASH_ITER(hh, g_queries, query, query_tmp) {
                if (query->expires_at_epoch_sec <= tv_now.tv_sec) {
                    // Remove the item from the hash
                    HASH_DEL(g_queries, query);
                    // Cancel the query, no callback will be made by libunbound
                    ub_cancel(ctx->ub_ctx, query->id);
                    // Mark the query as expired
                    query->expired = 1;
                    // Issue the callback ourselves
                    ub_reverse_lookup_callback(query, 1, NULL);
                } else {
                    break;
                }
            }

            // Release our write lock
            pthread_rwlock_unlock(&g_query_lock);
        }
    }

    // We're stopping - clean up the outstanding queries

    // Acquire a write lock
    if (pthread_rwlock_wrlock(&g_query_lock) != 0) {
        printf("unbound4j: Failed to acquire write lock.");
    }

    // Iterate through the outstanding queries
    HASH_ITER(hh, g_queries, query, query_tmp) {
        // Remove the item from the hash
        HASH_DEL(g_queries, query);
        // Cancel the query, no callback will be made by libunbound
        ub_cancel(ctx->ub_ctx, query->id);
        // Mark the query as expired
        query->expired = 1;
        // Issue the callback ourselves
        ub_reverse_lookup_callback(query, 1, NULL);
    }

    // Release our write lock
    pthread_rwlock_unlock(&g_query_lock);

    return NULL;
}
