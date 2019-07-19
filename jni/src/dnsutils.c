//
// Created by jesse on 19/07/19.
//

#include "dnsutils.h"

#include <stdio.h>
#include <string.h>

void build_reverse_lookup_domain_v4(struct in_addr* addr, char** res) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%u.%u.%u.%u.in-addr.arpa",
             (unsigned)((uint8_t*)addr)[3], (unsigned)((uint8_t*)addr)[2],
             (unsigned)((uint8_t*)addr)[1], (unsigned)((uint8_t*)addr)[0]);
    *res = strdup(buf);
}

void build_reverse_lookup_domain_v6(struct in6_addr* addr, char** res) {
    /* [nibble.]{32}.ip6.arpa. is less than 128 */
    const char* hex = "0123456789abcdef";
    char buf[128];
    char *p;
    int i;

    p = buf;
    for(i=15; i>=0; i--) {
        uint8_t b = ((uint8_t*)addr)[i];
        *p++ = hex[ (b&0x0f) ];
        *p++ = '.';
        *p++ = hex[ (b&0xf0) >> 4 ];
        *p++ = '.';
    }
    snprintf(buf+16*4, sizeof(buf)-16*4, "ip6.arpa");
    *res = strdup(buf);
}
