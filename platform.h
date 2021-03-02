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
 *
 * We overload this to support pointing to both functions and tables for the 
 * global search use case (hext is the pointer, and marked is the type.)
 *
 * We can't just point to an object since a C function is just a pointer and
 * there's no tt value we can lookup.
 */
typedef struct ro_TString {
    CommonHeader;
    lu_byte extra;
    lu_byte shrlen;
    unsigned int hash;
    union {
        size_t lnglen;
        struct TString *hext;
    } u;
    char contents[];
} ro_TString;


/**
 * Some macros to provide our overloaded access to the TString structure
 */
#define ro_has_func(o)  (o->marked == LUA_VCCL)
#define ro_func(o)      ((void *)((o)->u.hnext))
#define ro_has_table(o) (o->marked == LUA_TTABLE)
#define ro_table(o)     ((Table *)((o)->u.hnext))

/**
 * Our read-only strings are built by a perl script and written into the
 * ro_main.h file, which we include here.
 */
#define ROSTRING(s, tok) \
    { .next = NULL, .tt = LUA_VSHRSTR, .marked = 0, \
        .extra = tok,  \
        .shrlen = sizeof(s)-1, .contents = s }
#define ROFUNC(s, func) \
    { .next = NULL, .tt = LUA_VSHRSTR, .marked = LUA_VCCL, \
        .extra = 0, .u.hext = (TString *)func, \
        .shrlen = sizeof(s)-1, .contents = s }
#define ROTABLE(s, table) \
    { .next = NULL, .tt = LUA_VSHRSTR, .marked = LUA_TTABLE, \
        .extra = 0, .u.hext = (TString *)table, \
        .shrlen = sizeof(s)-1, .contents = s }

/**
 * Our read-only table is a standard table, but we re-use the node pointer
 * to point to a list of functions.
 */
#define DEFTABLE(name,list,count) \
    const Table name = { \
        .flags = 0, .alimit = count, .node = (void *)list };


int read_only_lookup(StkId ra, TValue *t, TValue *f);

int global_lookup(StkId ra, char *key);

TString *read_only_string(const char *str, size_t len);

#endif
