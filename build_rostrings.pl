#!/usr/bin/perl -w

require "./build_lib.pl";

#
# standard tokens and metamethod names to reduce all the RAM
# based strings
#
READ_TOKENS();
READ_WORDS("src/ltm.c", "luaT_eventname[]");

#
# Some strings that will be used, so may as well get them setup
#
ADD_WORD("_G");
# ADD_WORD("_ENV");     -- doesn't work because it does a luaC_fix on it!
ADD_WORD("count");
ADD_WORD("collect");

#
# lmathlib is all ok apart from "random" and "randomseed"
#
READ_LIB("mathlib", "src/lmathlib.c", "mathlib[]");
REMOVE_STRING("random");
REMOVE_STRING("randomseed");
ADD_FLOAT("pi", "PI");
ADD_FLOAT("huge", "(lua_Number)HUGE_VAL");
ADD_INT("maxinteger", "LUA_MAXINTEGER");
ADD_INT("mininteger", "LUA_MININTEGER");
PROCESS_LIB();
LIB_TABLE();

#
# lstrlib base functionality is handled, still TOOD is the
# string metatable.
#
READ_LIB("strlib", "src/lstrlib.c", "strlib[]");
PROCESS_LIB();
LIB_TABLE();

#
# lcorolib
#
READ_LIB("corolib", "src/lcorolib.c", "co_funcs[]");
PROCESS_LIB();
LIB_TABLE();

#
# lbaselib is mostly handled as normal, the _G entry will not be
# processed so will need to be added in initalisation if needed.
#
READ_LIB("baselib", "src/lbaselib.c", "base_funcs[]");
ADD_STRING("_VERSION", LUA_VERSION);
ADD_GLOBAL_TABLE("math", "mathlib");
ADD_GLOBAL_TABLE("string", "strlib");
ADD_GLOBAL_TABLE("coroutine", "corolib");
PROCESS_FINAL();



PROCESS_STRINGS();
