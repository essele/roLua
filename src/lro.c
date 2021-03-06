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
    //fprintf(stderr, "read_only_string(%s) (len=%zu\n", buf, len);

    TString *ts;

    while (start <= end) {
        unsigned int mid = start + ((end-start)/2);

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

/**
 * Process the data hidden in the TString and set the value as required
 */
void prepare_return(StkId ra, TString *ts) {
    switch(ts->hash) {
    
    case LUA_VCCL:
        setfvalue(s2v(ra), ro_func(ts));
        fprintf(stderr, "returning function\n");
        break;
    case LUA_TTABLE:
        val_(s2v(ra)).gc = obj2gco(ro_table(ts));
        settt_(s2v(ra), ctb(LUA_VTABLE));
        fprintf(stderr, "returning table\n");
        break;
    case LUA_VNUMFLT:
        setfltvalue(s2v(ra), ro_float(ts));
        fprintf(stderr, "returning float\n");
        break;
    case LUA_VNUMINT:
        setivalue(s2v(ra), ro_int(ts));
        fprintf(stderr, "returning int\n");
        break;
    case LUA_VSHRSTR:
        val_(s2v(ra)).gc = obj2gco(ro_string(ts));
        settt_(s2v(ra), ctb(LUA_VSHRSTR));
        fprintf(stderr, "returning string\n");
        break;
    default:
        fprintf(stderr, "not found\n");
        setnilvalue(s2v(ra));
        break;
    }
}

/**
 * Lookup for globals (called from lvm.c). We need to search baselib and
 * then look for specific library tables.
 */
int global_lookup(StkId ra, char *key) {

    // too short or too long...
    size_t len = strlen(key);
    if (len < MIN_BASELIB_LEN || len > MAX_BASELIB_LEN)
        return 0;

    TString *ts = find_in_list(ro_list_baselib, 0, MAX_BASELIB-1, key, len);
    if (!ts)
        return 0;

    fprintf(stderr, "hash is %d\n", ts->hash); 

    prepare_return(ra, ts);
    return 1;
}





/**
 * TODO: this is called when looking up a field within a table, so if the
 * table is read-only then it's one of our fake ones used for libraries.
 * 
 * We need to decode the *list* from the table, and then use the normal
 * searching function to find the function.
 */
// Lookup the field in the table...
int read_only_lookup(StkId ra, TValue *t, TValue *f) {
    // It needs to be a table...
    if (!ttistable(t)) return 0;
   
    Table   *table = hvalue(t);

    fprintf(stderr, "table lookup key [%s]\n", svalue(f));
    // It needs to be read only...
    //if (!is_read_only(table)) return 0;
    // TODO: see if we ever get a global table here

    // For a normal table the gclist will have a value in, so if it's
    // zero then it must be a read-only table
    if (table->gclist) return 0;

    fprintf(stderr, "read only table found\n");

    // Key must be a string... otherwise nil return
    if (!ttisstring(f)) {
        fprintf(stderr, "not string key\n");
        setnilvalue(s2v(ra));
        return 1;
    }

    // Locate the table and the size
    luaL_Reg    *reg = (luaL_Reg *)table->node;
    unsigned int count = table->alimit;

    fprintf(stderr, "Have pointer to funcs=%p  count=%d\n", (void *)reg, count);

    // Now see if we can find the key...
    char *key = svalue(f);
    fprintf(stderr, "looking for key [%s]\n", key);
    //int index = qfind(reg, count, key);

    TString *ts = find_in_list((const ro_TString **)reg, 0, count, key, strlen(key));
    fprintf(stderr, "find_in_list found %p\n", (void *)ts);
    
    if (!ts) {
        fprintf(stderr, "not found\n");
        setnilvalue(s2v(ra));
        return 1;
    }
    prepare_return(ra, ts);
    return 1;
    setfvalue(s2v(ra), ro_func(ts));
    fprintf(stderr, "returning function\n");
    return 1;

    

}

