/*
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <lapi.h>
#include <lstate.h>
#include <lobject.h>
#include <lgc.h>
*/

#include <stdlib.h>
#include <string.h>
#include "lro.h"

/*==========================================================================
 *
 * We use a rom based read-only string which is a replica of a TString.
 * This is used for any read only strings (reserved words, and meta stuff)
 * but also we overload it for the library functions, as they will end up
 * being strings at some point anyway.
 *
 * For a library function we will store the call in the u.hext position, we
 * then maintain one list for the read_only_string search function, and each
 * library then has a separate list to search for their specific functions.
 *==========================================================================
 */

#include "ro_main.h"

/**
 * This routine looks up a string to see if it's one of our pre-populated
 * read-only strings.
 *
 * We use a quick sort to make this a quick as possible, although it may
 * need some kind of hashing to speed this up further as the numbers
 * increase.
 */
TString *find_in_list(const ro_TString *list[], int start,
                                int end, const char *str, size_t len) {
    char buf[80];
    strncpy(buf, str, len);
    buf[len] = 0;
 //   fprintf(stderr, "find_in_list(%s) (len=%zu\n", buf, len);

    TString *ts;

    while (start <= end) {
        unsigned int mid = start + ((end-start)/2);
//        fprintf(stderr, "start=%d mid=%d end=%d\n", start, mid, end);

        ts = (TString *)list[mid];
//        fprintf(stderr, "looking at [%s]\n", ts->contents);

        // Check strings match first, then use length as decider...
        int cmp = memcmp(ts->contents, str, len);
        if (cmp == 0) {
            cmp = (ts->shrlen < len ? -1 : (ts->shrlen > len ? 1 : 0));
        }
    
        if (cmp == 0) {
//            fprintf(stderr, "found [%s]\n", ts->contents);
            return ts;
        }
        if (start == end) break;

        if (cmp < 0) {
            start = mid+1;
        } else if (cmp > 0) {
            end = mid-1;
        }
    }
    // Not found
//    fprintf(stderr, "string not found\n");
    return NULL;
}

/**
 * Specific lookup for read-only strings (called from internshrstr in
 * lstring.c)
 */
TString *read_only_string(const char *str, size_t len) {
    // If they don't fit within our length limits...
    if (len < MIN_ROSTRING_LEN || len > MAX_ROSTRING_LEN)
        return NULL;

    // We won't have any that are non-printable...
    if (*str < 33 || *str > 127)
        return NULL;

    // See if we have a range defined for the first character...
    const ro_range *range = &ro_range_lookup[(*str)-33];

    if (range->start < 0)
        return 0;

    return find_in_list(ro_tstrings, range->start, range->end, str, len);
}


/*
 * Prepare a TValue as a return value which will contain what's encoded into
 * the read-only TString.
 */
void prepare_ro_value(TString *ts, TValue *tv) {
    if (!ts) {
        setnilvalue(tv);
        return;
    }

    switch(ts->hash) {
    case LUA_VCCL:
        setfvalue(tv, ro_func(ts));
        break;
    case LUA_TTABLE:
        val_(tv).gc = obj2gco(ro_table(ts));
        settt_(tv, ctb(LUA_TTABLE));
        break;
    case LUA_VNUMFLT:
        setfltvalue(tv, ro_float(ts));
        break;
    case LUA_VNUMINT:
        setivalue(tv, ro_int(ts));
        break;
    case LUA_VSHRSTR:
        val_(tv).gc = obj2gco(ro_string(ts));
        settt_(tv, ctb(LUA_VSHRSTR));
        break;
    default:
        setnilvalue(tv);
        break;
    }
    return;
}

/**
 * Lookup for globals (called from ltable.c). Should only be called if we are looking
 * at the global table (marked.)
 */
TValue *global_ro_lookup(TString *key) {
    // HORRIBLE use of a static TValue, need to fix this
    static TValue   tv;
    char            *k = getstr(key);
    size_t          len = strlen(k);

    if (len < MIN_BASELIB_LEN || len > MAX_BASELIB_LEN) {
        setnilvalue(&tv);
        return &tv;
    }

    TString *ts = find_in_list(ro_list_baselib, 0, MAX_BASELIB-1, k, len);
    prepare_ro_value(ts, &tv);
    return &tv;
}


/**
 * Lookup within ro_tables (called from ltable.c)
 */
TValue *ro_table_lookup(Table *table, TString *key) {
    // HORRIBLE use of a static TValue, need to fix this
    static TValue tv;

    // Locate the table and the size
    luaL_Reg    *reg = (luaL_Reg *)table->node;
    unsigned int count = table->alimit;

    // Now see if we can find the key...
    char    *k = getstr(key);
    fprintf(stderr, "ROT: Have pointer to funcs=%p  count=%d\n", (void *)reg, count);
    fprintf(stderr, "ROT: looking for key [%s]\n", k);
    TString *ts = find_in_list((const ro_TString **)reg, 0, count-1, k, strlen(k));
    fprintf(stderr, "ROT: find_in_list found %p\n", (void *)ts);

    prepare_ro_value(ts, &tv);
    return &tv;
}

