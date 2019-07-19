//
// Created by jesse on 18/07/19.
//

#include <pthread.h>
#include <stdio.h>
#include <unbound.h>

#include "uthash.h"
#include "unbound4j.h"
#include "sldns.h"

struct ub4j_context *contexts = NULL;
pthread_rwlock_t g_lock;

void ub4j_init() {
    if (pthread_rwlock_init(&g_lock, NULL) != 0) {
        printf("unbound4j: Error while initializing read-write lock.\n");
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

struct ub4j_context* ub4j_create_context(char* error, size_t error_len) {
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

    // TODO: Read configuration from config object
    ub_ctx_set_fwd(ctx->ub_ctx, "127.0.0.1");

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
    if (pthread_rwlock_wrlock(&g_lock) != 0) {
        snprintf(error, error_len, "Failed to acquire write lock.");
        ctx->stopping = 1; // Stop the thread
        ub_ctx_delete(ctx->ub_ctx);
        return NULL;
    }
    HASH_ADD_INT(contexts, id, ctx);
    pthread_rwlock_unlock(&g_lock);
    return ctx;
}

int ub4j_delete_context(long ctx_id, char* error, size_t error_len) {
    // Acquire a write lock
    if (pthread_rwlock_wrlock(&g_lock) != 0) {
        snprintf(error, error_len, "Failed to acquire write lock.");
        return -1;
    }

    // Lookup the context by id and remove it from the hashmap if found
    long id = (long)ctx_id;
    struct ub4j_context *ctx = NULL;
    HASH_FIND_INT(contexts, &id, ctx);
    pthread_rwlock_unlock(&g_lock);
    if (ctx != NULL) {
        HASH_DEL(contexts, ctx);
    }

    // Release the lock
    pthread_rwlock_unlock(&g_lock);

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
    // free the result
    ub_resolve_free(result);

    // issue the delegate callback
    callback_context->callback(callback_context->userdata, hostname);
    free(callback_context);
}

int ub4j_reverse_lookup(long ctx_id, unsigned char* addr, size_t addr_len, void* userdata, ub4j_callback_type callback, char* error, size_t error_len) {
    // Acquire a read lock
    if (pthread_rwlock_rdlock(&g_lock) != 0) {
        snprintf(error, error_len, "Failed to acquire read lock.");
        return -1;
    }

    // Lookup the context by id
    long id = (long)ctx_id;
    struct ub4j_context *ctx = NULL;
    HASH_FIND_INT(contexts, &id, ctx);
    pthread_rwlock_unlock(&g_lock);
    if (ctx == NULL) {
        snprintf(error, error_len, "Invalid context id.");
        return -1;
    }

    ub4j_callback_context* callback_context = malloc(sizeof(ub4j_callback_context));
    if (callback_context == NULL) {
        snprintf(error, error_len, "Failed to allocate memory for callback context.");
        return -1;
    }

    memset(callback_context, 0, sizeof(ub4j_callback_context));
    callback_context->userdata = userdata;
    callback_context->callback = callback;

    // Issue the reverse lookup
    char target[256];
    snprintf(target, 256, "%d.%d.%d.%d.in-addr.arpa.", addr[3], addr[2], addr[1], addr[0]);
    int retval = ub_resolve_async(ctx->ub_ctx, target,
                              12 /* RR_TYPE_PTR */,
                              1 /* CLASS IN (internet) */,
                              callback_context,
                              ub_reverse_lookup_callback,
                              NULL);
    if (retval) {
        // Free the callback context
        free(callback_context);
        snprintf(error, error_len, "Resolve error: %s", ub_strerror(retval));
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
