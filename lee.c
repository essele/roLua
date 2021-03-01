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




luaL_Reg        *baselib = NULL;
extern const luaL_Reg base_funcs[];
unsigned int    baselib_count;
unsigned int    baselib_minlen;

void set_baselib(const luaL_Reg *lib, unsigned int count) {
    baselib = (luaL_Reg *)lib;
    baselib_count = count;
    fprintf(stderr, "bqselib set, count=%d\n", count);
    fprintf(stderr, "lib is %p, base is %p\n", (void *)baselib, (void *)base_funcs);
}


// Search through a luaL_Reg for a given name, but do it as quickly as possible.
// Performance testing shows this is better on average than a linear search right 
// down to lists of a couple of items, so not worth optimising any further.
//
int qfind(const luaL_Reg *list, unsigned int count, const char *item) {
    if (count == 0) return -1;

    fprintf(stderr, "count is %d\n", count);

    unsigned int end = count-1;     // zero base, and lose the NULL
    unsigned int start = 0;

    while (1) {
        unsigned int mid = start + ((end-start)/2);
        int cmp = strcmp(list[mid].name, item);

        if (start == end && cmp != 0) {
            break;
        }
        if (cmp < 0) {
            start = mid+1;
        } else if (cmp > 0) {
            end = mid-1;
        } else {
            return mid;
        }
    }
    return -1;
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

int global_lookup(StkId ra, char *key) {
    static int baselib_count = 0;
    static int baselib_minsize = 99;

    // Phase 1 -- look in the baselibs bit
    // Build the size the first time...
    if (!baselib_count) {
        luaL_Reg *p = (luaL_Reg *)&base_funcs;
        while (p->name) {
            int l = strlen(p->name);
            if (l < baselib_minsize) {
                baselib_minsize = l;
            }
            baselib_count++;
            p++;
        }
    }
    fprintf(stderr, "looking globally for %s\n", key);

    if (strlen(key) < baselib_minsize) {
        fprintf (stderr, "key too short\n");
        return 0;
    }

    int index = qfind(base_funcs, baselib_count, key);
    fprintf(stderr, "index is %d\n", index);

    if (index == -1) {
        return 0;
    }
    fprintf(stderr, "pointer is %p\n", (void *)base_funcs[index].func);

    setfvalue(s2v(ra), base_funcs[index].func);

    return 1;
}

// Lookup the field in the table...
int read_only_lookup(StkId ra, TValue *t, TValue *f) {
    // It needs to be a table...
    if (!ttistable(t)) return 0;
   
    Table   *table = hvalue(t);

    // It needs to be read only...
    if (!is_read_only(table)) return 0;
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
    int index = qfind(reg, count, key);
    
    fprintf(stderr, "index is %d\n", index); 
   
    if (index == -1) {
        fprintf(stderr, "not found\n");
        setnilvalue(s2v(ra));
        return 1;
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
    luaL_openlibs(L);
    fprintf(stderr, " ---- AFTER OPENLIBS ------\n");

    // Push the pointer to function
    lua_pushcfunction(L, multiplication);

    // Get the value on top of the stack
    // and set as a global, in this case is the function
    lua_setglobal(L, "mul");

    // Our Lua code, it simply prints a Hello, World message
    char * code = "collectgarbage('collect');print(collectgarbage('count'));x=mul(7,8);print(x);f={};f.a=1;f.b=2;print(f.a); print(x.joe)";

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




