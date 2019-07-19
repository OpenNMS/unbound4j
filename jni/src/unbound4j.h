//
// Created by jesse on 18/07/19.
//

#ifndef UNBOUND4J_UNBOUND4J_H
#define UNBOUND4J_UNBOUND4J_H

#include "uthash.h"

struct ub4j_config {
    short use_system_resolver;
    const char* unbound_config;
};

struct ub4j_context {
    long id;
    struct ub_ctx *ub_ctx;
    int ub_fd;
    pthread_t thread_id;
    volatile short stopping;
    UT_hash_handle hh; // makes this structure hashable
};

typedef void (*ub4j_callback_type)(void*, const char*, char*);

void ub4j_init();

void ub4j_destroy();

struct ub4j_context* ub4j_create_context(struct ub4j_config* config, char* error, size_t error_len);

int ub4j_delete_context(long ctx_id, char* error, size_t error_len);

int ub4j_reverse_lookup(long ctx_id, uint8_t* addr, size_t addr_len, void* mydata, ub4j_callback_type callback, char* error, size_t error_len);

void ub4j_print_stats(long ctx_id);

#endif //UNBOUND4J_UNBOUND4J_H
