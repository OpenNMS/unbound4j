//
// Created by jesse on 17/07/19.
//
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "unbound4j.h"

volatile short got_callback = 0;

void callback(void* mydata, const char* err_str, char* result) {
    if (err_str != NULL) {
        printf("Error: %s", err_str);
    } else if (result != NULL) {
        unsigned char* addr = (unsigned char*)mydata;
        printf("%d.%d.%d.%d = %s\n", addr[3], addr[2], addr[1], addr[0], result);
        free(result);
    } else {
        printf("(No result)");
    }
    got_callback = 1;
}

int main() {
    ub4j_init();

    struct ub4j_config config;
    memset(&config, 0, sizeof(struct ub4j_config));

    char error[256];
    size_t  error_len = sizeof(error);
    struct ub4j_context* ctx = ub4j_create_context(&config, error, error_len);

    if (ctx == NULL) {
        printf("Failed to create context: %s\n", error);
        ub4j_destroy();
        return 1;
    }

    //uint8_t addr[] = {1, 1, 1, 1};
    uint8_t addr[] = {192, 51, 100, 1};
    printf("Issuing reverse lookup for: %d.%d.%d.%d\n", addr[0], addr[1], addr[2], addr[3]);
    if (ub4j_reverse_lookup(ctx->id, addr, sizeof(addr), addr, callback, error, error_len)) {
        printf("Failed to issue reverse lookup: %s\n", error);
        ub4j_delete_context(ctx->id, error, error_len);
        ub4j_destroy();
        return 1;
    }

    while (1 > 0) {
        sleep(1);
        if (got_callback) {
            break;
        } else {
            printf(".");
            fflush(stdout);
        }
    }

    sleep(1);
    ub4j_print_stats(ctx->id);

    ub4j_destroy();
    return 0;
}
