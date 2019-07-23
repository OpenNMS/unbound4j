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

#include "dnsutils.h"

#include <stdio.h>
#include <string.h>

/**
 * Adapted from ./smallapp/unbound-host.c in the Unbound source tree.
 *
 * @param addr
 * @param res
 */
void build_reverse_lookup_domain_v4(struct in_addr* addr, char** res) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%u.%u.%u.%u.in-addr.arpa",
             (unsigned)((uint8_t*)addr)[3], (unsigned)((uint8_t*)addr)[2],
             (unsigned)((uint8_t*)addr)[1], (unsigned)((uint8_t*)addr)[0]);
    *res = strdup(buf);
}

/**
 * Adapted from ./smallapp/unbound-host.c in the Unbound source tree.
 *
 * @param addr
 * @param res
 */
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
