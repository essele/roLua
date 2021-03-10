/**
 * Include file needed to support read-only capabilities for strings and
 * tables.
 *
 * This will be included by any code that has need of the macros of
 * read_only related functions
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

#ifndef RO_LUA

// Ensure any of our read_only code is nulled out...
#define is_obj_ro(o)                (0)
#define is_global_table(t)          (0)
#define read_only_string(s,l)       (NULL)
#define ro_table_lookup(t, k)       (NULL)
#define global_ro_lookup(k)         (NULL)

#else

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
 * the global search use case (hnext is the pointer, and hash is the type.)
 *
 * For functions hnext points to the func, ints are cast into the pointer and
 * for everything else we point to a suitable object (or float)
 *
 *
 * We need to be able to define the strings as part of the libraries but the use if
 * different in different places.
 *
 * For any file which is a LUA_LIB we need the proper definitions of the RO_ items
 * that define things that are strings, funcs, or numbers, and they must also extern
 * themselves since they will be used elsewhere.
 *
 * The RO_LIST is a null event in LUA_LIB files, since they aren't used there at all.
 *
 * If we are not a LUA_LIB then it's the other way around.
 */

// Defines a variant of TString with the right length contents for the given string...

#define _ROTST(s)  struct { CommonHeader; lu_byte extra; lu_byte shrlen; unsigned int hash; \
                      union { size_t lnglen; struct TString *hnext; } u; \
                                char contents[(sizeof(s)-1)]; }

typedef struct { CommonHeader; lu_byte extra; lu_byte shrlen; unsigned int hash;
                      union { size_t lnglen; struct TString *hnext; } u;
                                char contents[]; } ro_TString;

// Defines a basic word used for word lookups, with no other related info...
#define RO_WORD(s)      { .shrlen=sizeof(s)-1, .contents=s, .tt = LUA_VSHRSTR, }

// This is a token where the token number is also stored...
#define RO_TOKEN(s, t)  { .shrlen=sizeof(s)-1, .contents=s, .tt = LUA_VSHRSTR, .extra = t, }

// Now a function...
#define RO_FUNC(s, fv)  { .shrlen=sizeof(s)-1, .contents=s, \
                                    .tt = LUA_VSHRSTR, .hash = LUA_VCCL, \
                                    .u.hnext = (TString *)fv }

// And integer...
#define RO_INT(s, iv)   { .shrlen=sizeof(s)-1, .contents=s, \
                                    .tt = LUA_VSHRSTR, .hash = LUA_VNUMINT, \
                                    .u.hnext = (TString *)&(Value){ .i = iv } }

// And float...
#define RO_NUM(s, nv)   { .shrlen=sizeof(s)-1, .contents=s, \
                                    .tt = LUA_VSHRSTR, .hash = LUA_VNUMFLT, \
                                    .u.hnext = (TString *)&(Value){ .n = nv } }

// And string...
#define RO_STRING(s, sv)    { .shrlen=sizeof(s)-1, .contents=s, \
                                    .tt = LUA_VSHRSTR, .hash = LUA_VSHRSTR, \
                                    .u.hnext = (TString *)&(_ROTST(sv)){ .shrlen=sizeof(sv)-1, \
                                    .contents=sv, .tt = LUA_VSHRSTR } }

// And table...
#define RO_TABLE(s, tv)     { .shrlen=sizeof(s)-1, .contents=s, \
                                    .tt = LUA_VSHRSTR, .hash = LUA_VTABLE, \
                                    .u.hnext = (TString *)&(Table){ \
                                        .alimit = sizeof(tv)/sizeof(void *), \
                                        .node = (void *)&tv, \
                                    } }



/**
 * The include files are used in two different places and things need to behave differently
 *
 * 1. In the lib.c files in which case we need externs and definitions of all the TStrings
 *    apart from TABLE's (which need to be where the lists are) and lists.
 * 2. In the lro.c file, where most of just externs, but we need tables and lists.
 */
#ifdef LUA_LIB
#define RO_DEF_FUNC(n, s, fv)        extern const ro_TString n; const ro_TString n = RO_FUNC(s, fv)
#define RO_DEF_INT(n, s, iv)         extern const ro_TString n; const ro_TString n = RO_INT(s, iv)
#define RO_DEF_NUM(n, s, nv)         extern const ro_TString n; const ro_TString n = RO_NUM(s, nv)
#define RO_DEF_STRING(n, s, sv)      extern const ro_TString n; const ro_TString n = RO_STRING(s, sv)
#define RO_DEF_TABLE(n, s, tv)       
#define RO_DEF_LIST(n, ...)
#else
#define RO_DEF_FUNC(n, s, fv)        extern const ro_TString n
#define RO_DEF_INT(n, s, iv)         extern const ro_TString n
#define RO_DEF_NUM(n, s, nv)         extern const ro_TString n
#define RO_DEF_STRING(n, s, sv)      extern const ro_TString n
#define RO_DEF_TABLE(n, s, tv)       const ro_TString n = RO_TABLE(s, tv)
#define RO_DEF_LIST(n, ...)          const ro_TString * const n[] = { __VA_ARGS__ }
#endif


/**
 * Some macros to provide our overloaded access to the TString structure
 */
#define ro_func(o)      ((void *)((o)->u.hnext))
#define ro_table(o)     ((Table *)((o)->u.hnext))
#define ro_float(o)     (*(lua_Number *)((o)->u.hnext))
#define ro_string(o)    ((TString *)((o)->u.hnext))
#define ro_int(o)       (*(lua_Integer *)((o)->u.hnext))

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
#define XXROSTRING(s, tok, ttt, item) \
    { .next = NULL, .tt = LUA_VSHRSTR, .hash = ttt, \
        .extra = tok, .u.hnext = (TString *)item, \
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

#endif      // RO_LUA
#endif      // lro_h
