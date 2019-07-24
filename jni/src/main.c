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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unbound.h>

#include "unbound4j.h"

/*
 * Used to generate a large number of reverse lookup requests to
 * test the system under load.
 */

volatile int done = 0;
atomic_int ip_generator = ATOMIC_VAR_INIT(16843009); // Start at 1.1.1.1

void callback(void* mydata, const char* err_str, char* result) {
    if (err_str != NULL) {
        printf("Error: %s\n", err_str);
    } else if (result != NULL) {
        printf("Result: %s\n", result);
        free(result);
    } else {
        printf("(No result)\n");
    }
}

void *thread_routine_r( void *arg ) {
    struct ub4j_context* ctx = (struct ub4j_context*)arg;

    char error_str[256];
    size_t error_len = sizeof(error_str);
    while(!done) {
        // Generate 3 IP address
        int ips[3];
        ips[0] = atomic_fetch_add(&ip_generator, 1);
        ips[1] = atomic_fetch_add(&ip_generator, 1);
        ips[2] = atomic_fetch_add(&ip_generator, 1);

        // Perform lookups for each of these
        for (int i = 0; i < 3; i++) {
            if (ub4j_reverse_lookup(ctx->id, (uint8_t*)(&(ips[i])), 4, NULL, callback, error_str, error_len)) {
                printf("lookup failed: %s", error_str);
            }
        }
    }
}

int main() {
    ub4j_init();

    printf("libunbound v%s\n", ub_version());

    struct ub4j_config config;
    ub4j_config_init(&config);

    char error[256];
    size_t  error_len = sizeof(error);
    struct ub4j_context* ctx = ub4j_create_context(&config, error, error_len);

    if (ctx == NULL) {
        printf("Failed to create context: %s\n", error);
        ub4j_destroy();
        return 1;
    }

    // Spawn the lookup threads
    const int NUM_THREADS = 12;
    pthread_t dns_lookup_threads[NUM_THREADS];
    int status;
    for (int i = 0; i < NUM_THREADS; i++) {
        if (( status = pthread_create( &(dns_lookup_threads[i]), NULL, thread_routine_r, ctx) )) {
            printf("failure: status %d\n", status);
            return 1;
        }
    }

    // Wait
    sleep(10);

    // Stop
    done = 1;
    for (int i = 0; i < NUM_THREADS; i++) {
        void *thread_result;
        status = pthread_join( dns_lookup_threads[i], &thread_result );
        printf("thread result: %d %ld\n", status, (long)thread_result);
    }

    ub4j_destroy();
    return 0;
}
