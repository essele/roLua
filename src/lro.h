/**
 * Include file needed to support read-only capabilities for strings and
 * tables.
 */

#ifndef lro_h
#define lro_h

#include "lapi.h"
#include "lauxlib.h"

/*
 * Define RO_LUA in order to have low memory usage (for easy switching on
 * and off)
 */
#define RO_LUA

/*
 * Anything that's an object and read-only will have a "next" field that's
 * zero, where any genuine object will be on the allgc list.
 *
 * the global table will be marked using but 7 (normally used for testing)
 */
#define is_obj_ro(o)            (!(o)->next)
#define is_global_table(t)      ((t)->marked & 0x80)

/**
 * Read-only strings... this needs to be redefined so that we can include the
 * full content in the structure in the rom image.
 *
 * We overload this to support pointing to both functions and other object for
 * the global search use case (hext is the pointer, and hash is the type.)
 *
 * For functions hext points to the func, ints are cast into the pointer and
 * for everything else we point to a suitable object (or float)
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
#define ro_func(o)      ((void *)((o)->u.hnext))
#define ro_table(o)     ((Table *)((o)->u.hnext))
#define ro_float(o)     (*(lua_Number *)((o)->u.hnext))
#define ro_string(o)    ((TString *)((o)->u.hnext))
#define ro_int(o)       ((LUA_INTEGER)((o)->u.hnext))

/**
 * Helper type to speed up the binary search by limiting to the range of
 * items with a matching first char, also rules out any char not contained
 * in the word list.
 */
typedef struct ro_range {
    int16_t start;
    int16_t end;
} __attribute__((packed)) ro_range;

/**
 * Our read-only strings are built by a perl script and written into the
 * ro_<name>.h files.
 */
#define ROSTRING(s, tok, ttt, item) \
    { .next = NULL, .tt = LUA_VSHRSTR, .hash = ttt, \
        .extra = tok, .u.hext = (TString *)item, \
        .shrlen = sizeof(s)-1, .contents = s }

/**
 * Our read-only table is a standard table, but we re-use the node pointer
 * to point to a list of functions.
 */
#define DEFTABLE(name,list,count) \
    const Table name = { \
        .flags = 0, .alimit = count, .node = (void *)list };

/**
 * Called from linit.c
 */
void ro_lua_init(lua_State *L);

/**
 * Used by lvm.c -> OP_GETFIELD
 */
int read_only_lookup(StkId ra, TValue *t, TValue *f);

TValue *ro_table_lookup(Table *table, TString *key);

TValue *global_ro_lookup(TString *key);
/**
 * Used by lvm.c -> OP_GETTABUP
 */
int global_lookup(StkId ra, char *key);

/**
 * Used by lstring.c -> internshrstr
 */
TString *read_only_string(const char *str, size_t len);

int ro_lua_next(lua_State *L, Table *t, StkId key);

#endif
