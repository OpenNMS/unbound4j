//
// Created by jesse on 19/07/19.
//

#ifndef UNBOUND4J_DNSUTILS_H
#define UNBOUND4J_DNSUTILS_H

#include <arpa/inet.h>

void build_reverse_lookup_domain_v4(struct in_addr* addr, char** res);
void build_reverse_lookup_domain_v6(struct in6_addr* addr, char** res);

#endif //UNBOUND4J_DNSUTILS_H
