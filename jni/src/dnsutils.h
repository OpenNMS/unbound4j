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

#ifndef UNBOUND4J_DNSUTILS_H
#define UNBOUND4J_DNSUTILS_H

#include <arpa/inet.h>

void build_reverse_lookup_domain_v4(struct in_addr* addr, char** res);
void build_reverse_lookup_domain_v6(struct in6_addr* addr, char** res);

#endif //UNBOUND4J_DNSUTILS_H
