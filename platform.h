/**
 * Specific include file to support the small number of platform specific
 * needs.
 *
 * Currently:
 *
 * 1. Be able to identify whether an item (void *) is in read-only memory
 *    or not.
 *
 */

#ifndef platform_h
#define platform_h

#include "src/lapi.h"
#include "src/lauxlib.h"

/*
 * Hack for x86 since we don't actually have any read-only memory
 */

extern void *read_only_list[];
int is_read_only(void *p);

/**
 * Read-only strings... this needs to be redefined so that we can include the
 * full content in the structure in the rom image.
 */
typedef struct roTString {
    CommonHeader;
    lu_byte extra;
    lu_byte shrlen;
    unsigned int hash;
    union {
        size_t lnglen;
        struct TString *hext;
    } u;
    char contents[];
} roTString;

/**
 * Our read-only strings are built by a perl script and written into the
 * ro_main.h file, which we include here.
 */
#define ROSTRING(s, tok) \
    { .next = NULL, .tt = LUA_VSHRSTR, .marked = 4, \
        .extra = tok,  \
        .shrlen = sizeof(s)-1, .contents = s }
#define ROFUNC(s, func) \
    { .next = NULL, .tt = LUA_VSHRSTR, .marked = 4, \
        .extra = 0, .u.hext = (TString *)func, \
        .shrlen = sizeof(s)-1, .contents = s }


int read_only_lookup(StkId ra, TValue *t, TValue *f);

int global_lookup(StkId ra, char *key);

TString *read_only_string(const char *str, size_t len);

#endif
