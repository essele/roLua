#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <lapi.h>
#include <lstate.h>
#include <lobject.h>
#include <lgc.h>

#include <stdlib.h>
#include <string.h>


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
//    lua_pushcfunction(L, multiplication);

    // Get the value on top of the stack
    // and set as a global, in this case is the function
//    lua_setglobal(L, "mul");

    // Our Lua code, it simply prints a Hello, World message
    char * code = 
//            "print(collectgarbage('count'));"
//            "collectgarbage('collect');"
//            "print(collectgarbage('count'));"
//            "x=mul(7,8);print(x);f={};f.a=1;f.b=2;print(f.a); print(x.joe);"
//            "f={};f.a=1;f.b=2;"
//            "print(f.a);"
            "for i=1,10 do print('hello') end;"
            "print(math.pi);"
            "print(math.floor(4.567));"
//            "x=nil; f=nil;"
//            "collectgarbage('collect');"
//            "print(collectgarbage('count'));"
            "collectgarbage('collect');"
            "print(collectgarbage('count'));"
            "print(math.huge);"
            "print(math.maxinteger);"
            "print(string);"
            "print(string.upper);"
            "print(string.upper('Lee Essen'));"
            "print(_VERSION);"
            "print(_G);"
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




