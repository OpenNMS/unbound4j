//
// Created by jesse on 17/07/19.
//

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include "sldns.h"

int sldns_str_vprint(char** str, size_t* slen, const char* format, va_list args)
{
    int w = vsnprintf(*str, *slen, format, args);
    if(w < 0) {
        /* error in printout */
        return 0;
    } else if((size_t)w >= *slen) {
        *str = NULL; /* we do not want str to point outside of buffer*/
        *slen = 0;
    } else {
        *str += w;
        *slen -= w;
    }
    return w;
}

int sldns_str_print(char** str, size_t* slen, const char* format, ...)
{
    int w;
    va_list args;
    va_start(args, format);
    w = sldns_str_vprint(str, slen, format, args);
    va_end(args);
    return w;
}

static int str_char_print(char** s, size_t* sl, uint8_t c)
{
    if(isprint((unsigned char)c) || c == '\t') {
        if(c == '\"' || c == '\\')
            return sldns_str_print(s, sl, "\\%c", c);
        if(*sl) {
            **s = (char)c;
            (*s)++;
            (*sl)--;
        }
        return 1;
    }
    return sldns_str_print(s, sl, "\\%03u", (unsigned)c);
}

int sldns_wire2str_str_scan(uint8_t** d, size_t* dl, char** s, size_t* sl)
{
    int w = 0;
    size_t i, len;
    if(*dl < 1) return -1;
    len = **d;
    if(*dl < 1+len) return -1;
    (*d)++;
    (*dl)--;
    w += sldns_str_print(s, sl, "\"");
    for(i=0; i<len; i++)
        w += str_char_print(s, sl, (*d)[i]);
    w += sldns_str_print(s, sl, "\"");
    (*d)+=len;
    (*dl)-=len;
    return w;
}

/** print and escape one character for a domain dname */
static int dname_char_print(char** s, size_t* slen, uint8_t c)
{
    if(c == '.' || c == ';' || c == '(' || c == ')' || c == '\\')
        return sldns_str_print(s, slen, "\\%c", c);
    else if(!(isascii((unsigned char)c) && isgraph((unsigned char)c)))
        return sldns_str_print(s, slen, "\\%03u", (unsigned)c);
    /* plain printout */
    if(*slen) {
        **s = (char)c;
        (*s)++;
        (*slen)--;
    }
    return 1;
}

int sldns_wire2str_dname_scan(uint8_t** d, size_t* dlen, char** s, size_t* slen,
                              uint8_t* pkt, size_t pktlen)
{
    int w = 0;
    /* spool labels onto the string, use compression if its there */
    uint8_t* pos = *d;
    unsigned i, counter=0;
    const unsigned maxcompr = 1000; /* loop detection, max compr ptrs */
    int in_buf = 1;
    if(*dlen == 0) return sldns_str_print(s, slen, "ErrorMissingDname");
    if(*pos == 0) {
        (*d)++;
        (*dlen)--;
        return sldns_str_print(s, slen, ".");
    }
    while(*pos) {
        /* read label length */
        uint8_t labellen = *pos++;
        if(in_buf) { (*d)++; (*dlen)--; }

        /* find out what sort of label we have */
        if((labellen&0xc0) == 0xc0) {
            /* compressed */
            uint16_t target = 0;
            if(in_buf && *dlen == 0)
                return w + sldns_str_print(s, slen,
                                           "ErrorPartialDname");
            else if(!in_buf && pos+1 > pkt+pktlen)
                return w + sldns_str_print(s, slen,
                                           "ErrorPartialDname");
            target = ((labellen&0x3f)<<8) | *pos;
            if(in_buf) { (*d)++; (*dlen)--; }
            /* move to target, if possible */
            if(!pkt || target >= pktlen)
                return w + sldns_str_print(s, slen,
                                           "ErrorComprPtrOutOfBounds");
            if(counter++ > maxcompr)
                return w + sldns_str_print(s, slen,
                                           "ErrorComprPtrLooped");
            in_buf = 0;
            pos = pkt+target;
            continue;
        } else if((labellen&0xc0)) {
            /* notimpl label type */
            w += sldns_str_print(s, slen,
                                 "ErrorLABELTYPE%xIsUnknown",
                                 (int)(labellen&0xc0));
            return w;
        }

        /* spool label characters, end with '.' */
        if(in_buf && *dlen < (size_t)labellen)
            labellen = (uint8_t)*dlen;
        else if(!in_buf && pos+(size_t)labellen > pkt+pktlen)
            labellen = (uint8_t)(pkt + pktlen - pos);
        for(i=0; i<(unsigned)labellen; i++) {
            w += dname_char_print(s, slen, *pos++);
        }
        if(in_buf) {
            (*d) += labellen;
            (*dlen) -= labellen;
            if(*dlen == 0) break;
        }
        w += sldns_str_print(s, slen, ".");
    }
    /* skip over final root label */
    if(in_buf && *dlen > 0) { (*d)++; (*dlen)--; }
    /* in case we printed no labels, terminate dname */
    if(w == 0) w += sldns_str_print(s, slen, ".");
    return w;
}

int sldns_wire2str_rdata_scan(uint8_t** d, size_t* dlen, char** s,
                              size_t* slen, uint16_t rrtype, uint8_t* pkt, size_t pktlen)
{
   return sldns_wire2str_dname_scan(d, dlen, s, slen, pkt, pktlen);
}

int sldns_wire2str_rdata_buf(uint8_t* rdata, size_t rdata_len, char* str,
                             size_t str_len, uint16_t rrtype)
{
    /* use arguments as temporary variables */
    return sldns_wire2str_rdata_scan(&rdata, &rdata_len, &str, &str_len,
                                     rrtype, NULL, 0);
}
