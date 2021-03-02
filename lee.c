#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <lapi.h>
#include <lstate.h>
#include <lobject.h>
#include <lgc.h>

#include <stdlib.h>
#include <string.h>

#include "platform.h"

#include <malloc/malloc.h>

#include <lstring.h>

static void *lee_alloc (void *ud, void *ptr, size_t osize, size_t nsize) {

    static size_t running_total = 0;

  (void)ud; (void)osize;  /* not used */
  if (nsize == 0) {
    size_t sz = malloc_size(ptr);
    running_total -= sz;
    fprintf(stderr, "freel called on %p (size=%zu) tot=%zu\n", ptr, sz, running_total);
    free(ptr);
    return NULL;
  }
  else {
    size_t sz = 0;
    if (ptr != 0) {
        sz = malloc_size(ptr);
        running_total -= sz;
    }
    running_total += nsize;
    fprintf(stderr, "realloc called on %p (%zu -> %zu) tot=%zu\n", ptr, sz, nsize, running_total);
    return realloc(ptr, nsize);
  }
}



// Dummy set of functions...
static const luaL_Reg sample_funcs[] = {
    { "fred", (void *)100 },
    { "joe", (void *)200 },
    { NULL, NULL },
};

// Dummy read-only table...
const Table   realt = {
    .flags = 5,
    .alimit = (sizeof(sample_funcs)/(2*sizeof(void *)))-1,
    .node = (void *)&sample_funcs,
};
// Dummy read-only table...
const Table   myTable = {
    .flags = 5,
    .alimit = (sizeof(sample_funcs)/(2*sizeof(void *)))-1,
    .node = (void *)&sample_funcs,
};


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
    fprintf(stderr, "read_only_string(%s) (len=%zu\n", buf, len);

//    unsigned int end = size-1;     // zero base, and lose the NULL
////    unsigned int start = 0;

    TString *ts;

    while (start <= end) {
        unsigned int mid = start + ((end-start)/2);

        ts = (TString *)list[mid];
        fprintf(stderr, "looking at [%s]\n", ts->contents);

        // Check strings match first, then use length as decider...
        int cmp = memcmp(ts->contents, str, len);
        if (cmp == 0) {
            cmp = (ts->shrlen < len ? -1 : (ts->shrlen > len ? 1 : 0));
        }
    
        if (cmp == 0) {
            fprintf(stderr, "found [%s]\n", ts->contents);
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
    fprintf(stderr, "string not found\n");
    return NULL;
}

/**
 * Specific lookup for read-only strings (called from internshrstr in
 * lstring.c)
 */
TString *read_only_string(const char *str, size_t len) {
    if (len < MIN_ROSTRING_LEN || len > MAX_ROSTRING_LEN)
        return NULL;

    return find_in_list(ro_tstrings, 0, MAX_ROSTRINGS-1, str, len);
}

/**
 * Lookup for globals (called from lvm.c). We need to search baselib and
 * then look for specific library tables.
 */
int global_lookup(StkId ra, char *key) {
    size_t len = strlen(key);

    if (len < MIN_BASELIB_LEN || len > MAX_BASELIB_LEN)
        return 0;

    TString *ts = find_in_list(ro_list_baselib, 0, MAX_BASELIB-1, key, len);
    if (!ts)
        return 0;

    if (ro_has_func(ts)) {
        setfvalue(s2v(ra), ro_func(ts));
        fprintf(stderr, "returning function\n");
    } else if (ro_has_table(ts)) {
        // sethvalue() (our own version)
        val_(s2v(ra)).gc = obj2gco(ro_table(ts));
        settt_(s2v(ra), ctb(LUA_VTABLE));
        //sethvalue(s2v(ra), ro_table(ts));
        fprintf(stderr, "returning table\n");
    }
    return 1;
}





/**
 * x86 hacks to simulate the read only stuff....
 */
void *read_only_list[] = {
    (void *)&realt,
    NULL,
};

int is_read_only(void *p) {
    void **list = read_only_list;
    
    while (*list) {
        if (*list++ == p) {
            return 1;
        }
    }
    return 0;
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
    return 0;
    if (!is_read_only(table)) return 0;
    // TODO: see if we ever get a global table here
//    if (table != &ro_table_mathlib) return 0;
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
    setfvalue(s2v(ra), ro_func(ts));
    fprintf(stderr, "returning function\n");
    return 1;

    

    int index = 0;
    
    fprintf(stderr, "index is %d\n", index); 
   
    if (index == -1) {
    } 
    int value = (int)sample_funcs[index].func;
    
    setivalue(s2v(ra), value);

    return 1; 
}



static const luaL_Reg some_funcs[] = {
    { "abc", NULL },
    { "bbc", NULL },
    { "cbc", NULL },
    { "dsdfsdbc", NULL },
    { "ebc", NULL },
    { "fbc", NULL },
    { "gbc", NULL },
    { "hbc", NULL },
    { "ibc", NULL },
    { "jbc", NULL },
    { "kbc", NULL },
    { "lbc", NULL },
    { "mbc", NULL },
    { "nbc", NULL },
    { NULL, NULL }
};

// TWO MAIN HOOKS INTO LUA
//
// 1. When looking up a table that is the global table, we need to interject
//    all of the global functions and also the sub-tables for the libraries
//
// 2. When looking up a read-only table (sub-table) for the libraries
//



// Define our function, we have to follow the protocol of lua_CFunction that is 
// typedef int (*lua_CFunction) (lua_State *L);
int blah(lua_State *L) {
    fprintf(stderr, "blah addr=%p\n", (void *)blah);

    // Check first argument is integer and return the value
    int a = luaL_checkinteger(L, 1);

    // Check second argument is integer and return the value
    int b = luaL_checkinteger(L, 2);

    // multiply and store the result inside a type lua_Integer
    lua_Integer c = a * b;

    // push the result to Lua
    lua_pushinteger(L, c);

    // exit code, successful = 1, otherwise error.
    return 1; // Successful
}

int multiplication(lua_State *L) {
    fprintf(stderr, "mul addr=%p\n", (void *)multiplication);

    // Check first argument is integer and return the value
    int a = luaL_checkinteger(L, 1);

    // Check second argument is integer and return the value
    int b = luaL_checkinteger(L, 2);

    // multiply and store the result inside a type lua_Integer
    lua_Integer c = a * b;
  
    Table *t = (Table *)&realt; 

    sethvalue(L, s2v(L->top), t);
    api_incr_top(L);

    // push the result to Lua
//    lua_pushinteger(L, c);
//    lua_pushcfunction(L, blah);

    // exit code, successful = 1, otherwise error.
    return 1; // Successful
}





int main(int argc, char ** argv) {


    lua_State *L = lua_newstate(lee_alloc, NULL);

    fprintf(stderr, " ---- AFTER NEWSTATE ------\n");



    TString *s = luaS_new(L, "goto");
    uint8_t *p = (uint8_t *)s;
    fprintf(stderr, "DUMP(%p) ", p);
    for (int i=0; i < sizeof(TString)+4; i++) {
        fprintf(stderr, "%02x ", *(p+i));
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "%08x\n", s->hash);
//    exit(0);



    luaL_openlibs(L);
    fprintf(stderr, " ---- AFTER OPENLIBS ------\n");

    // Push the pointer to function
    lua_pushcfunction(L, multiplication);

    // Get the value on top of the stack
    // and set as a global, in this case is the function
    lua_setglobal(L, "mul");

    // Our Lua code, it simply prints a Hello, World message
    char * code = 
            "collectgarbage('collect');print(collectgarbage('count'));x=mul(7,8);print(x);f={};f.a=1;f.b=2;print(f.a); print(x.joe);"
//            "for i=1,10 do print('hello') end;"
//            "print(math);"
//            "print(math.floor(4.567));"
            "x=nil; f=nil;"
            "collectgarbage('collect');"
            "print(collectgarbage('count'));"
            ;
        

    // Here we load the string and use lua_pcall for run the code
    fprintf(stderr, " ---- BEFORE LOADSTRING ------\n");
    if (luaL_loadstring(L, code) == LUA_OK) {
    fprintf(stderr, " ---- AFTER LOADSTRING ------\n");
        if (lua_pcall(L, 0, 1, 0) == LUA_OK) {
            // If it was executed successfuly we 
            // remove the code from the stack
            lua_pop(L, lua_gettop(L));
        }
    }

    lua_close(L);
    return 0;
}




