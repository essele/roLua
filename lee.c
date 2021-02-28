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

const Table   realt = {
    .flags = 5
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

// Search through a luaL_Reg for a given name, but do it as quickly as possible.
// Performance testing shows this is better on average than a linear search right 
// down to lists of a couple of items, so not worth optimising any further.
//
int qfind(const luaL_Reg *list, unsigned int count, const char *item) {
    if (count <= 1) return -1;

    unsigned int end = count-1;
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


    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    // Push the pointer to function
    lua_pushcfunction(L, multiplication);

    // Get the value on top of the stack
    // and set as a global, in this case is the function
    lua_setglobal(L, "mul");

    // Our Lua code, it simply prints a Hello, World message
    char * code = "print(collectgarbage('count'));x=mul(7,8);print(x);print(x.fred)";

    // Here we load the string and use lua_pcall for run the code
    if (luaL_loadstring(L, code) == LUA_OK) {
        if (lua_pcall(L, 0, 1, 0) == LUA_OK) {
            // If it was executed successfuly we 
            // remove the code from the stack
            lua_pop(L, lua_gettop(L));
        }
    }

    lua_close(L);
    return 0;
}




