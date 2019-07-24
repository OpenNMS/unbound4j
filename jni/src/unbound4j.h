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

#ifndef UNBOUND4J_UNBOUND4J_H
#define UNBOUND4J_UNBOUND4J_H

#include "uthash.h"

struct ub4j_config {
    short use_system_resolver;
    const char* unbound_config;
    int request_timeout_secs;
};

struct ub4j_context {
    int id;
    struct ub_ctx *ub_ctx;
    int ub_fd;
    pthread_t thread_id;
    volatile short stopping;
    int request_timeout_secs;
    UT_hash_handle hh; // makes this structure hashable
};

typedef void (*ub4j_callback_type)(void*, const char*, char*);

void ub4j_init();

void ub4j_destroy();

void ub4j_config_init(struct ub4j_config* config);

struct ub4j_context* ub4j_create_context(struct ub4j_config* config, char* error, size_t error_len);

int ub4j_delete_context(int ctx_id, char* error, size_t error_len);

int ub4j_reverse_lookup(int ctx_id, uint8_t* addr, size_t addr_len, void* mydata, ub4j_callback_type callback, char* error, size_t error_len);

#endif //UNBOUND4J_UNBOUND4J_H
