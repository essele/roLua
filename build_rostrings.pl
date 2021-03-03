#!/usr/bin/perl -w

require "./build_lib.pl";

READ_TOKENS();
READ_WORDS("src/ltm.c", "luaT_eventname[]");

READ_LIB("mathlib", "src/lmathlib.c", "mathlib[]");
ADD_FLOAT("pi", "PI");
ADD_FLOAT("huge", "(lua_Number)HUGE_VAL");
ADD_INT("maxinteger", "LUA_MAXINTEGER");
ADD_INT("mininteger", "LUA_MININTEGER");
PROCESS_LIB();


READ_LIB("baselib", "src/lbaselib.c", "base_funcs[]");
ADD_TABLE("math", "mathlib");
PROCESS_FINAL();

PROCESS_STRINGS();
