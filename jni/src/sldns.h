//
// Created by jesse on 17/07/19.
//

#ifndef ASYNCDNS4J_SLDNS_H
#define ASYNCDNS4J_SLDNS_H

#include <stdint.h>
#include <stddef.h>

int sldns_wire2str_str_scan(uint8_t** d, size_t* dl, char** s, size_t* sl);

int sldns_wire2str_rdata_buf(uint8_t* rdata, size_t rdata_len, char* str,
                             size_t str_len, uint16_t rrtype);

#endif //ASYNCDNS4J_SLDNS_H
