#!/usr/bin/perl -w

require "./build_lib.pl";

%INCLIBS=(
    "CORO" => undef,
    "UTF8" => undef,
    "DB" => undef,
    "MATH" => 1,
    "STRING" => 1,
    "TABLE" => 1,
    "UTF8" => undef,
);

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
if (defined $INCLIBS{"MATH"}) {
    READ_LIB("mathlib", "src/lmathlib.c", "mathlib[]");
    REMOVE_STRING("random");
    REMOVE_STRING("randomseed");
    ADD_FLOAT("pi", "PI");
    ADD_FLOAT("huge", "(lua_Number)HUGE_VAL");
    ADD_INT("maxinteger", "LUA_MAXINTEGER");
    ADD_INT("mininteger", "LUA_MININTEGER");
    PROCESS_LIB();
    LIB_TABLE();
}

#
# lstrlib base functionality is handled, still TOOD is the
# string metatable.
#
if (defined $INCLIBS{"STRING"}) {
    READ_LIB("strlib", "src/lstrlib.c", "strlib[]");
    PROCESS_LIB();
    LIB_TABLE();
}

#
# lcorolib
#
if (defined $INCLIBS{"CORO"}) {
    READ_LIB("corolib", "src/lcorolib.c", "co_funcs[]");
    PROCESS_LIB();
    LIB_TABLE();
}

#
# ltablib
#
if (defined $INCLIBS{"TABLE"}) {
    READ_LIB("tablib", "src/ltablib.c", "tab_funcs[]");
    PROCESS_LIB();
    LIB_TABLE();
}

#
# lutf8lib
#
if (defined $INCLIBS{"UTF8"}) {
    READ_LIB("utf8lib", "src/lutf8lib.c", "funcs[]");
    ADD_STRING("charpattern", UTF8PATT);
    PROCESS_LIB();
    LIB_TABLE();
}

#
# ldblib
#
if (defined $INCLIBS{"DEBUG"}) {
    READ_LIB("dblib", "src/ldblib.c", "dblib[]");
    PROCESS_LIB();
    LIB_TABLE();
}

#
# lbaselib is mostly handled as normal, the _G entry will not be
# processed so will need to be added in initalisation if needed.
#
READ_LIB("baselib", "src/lbaselib.c", "base_funcs[]");
ADD_STRING("_VERSION", LUA_VERSION);
ADD_GLOBAL_TABLE("math", "mathlib") if (defined $INCLIBS{"MATH"});
ADD_GLOBAL_TABLE("string", "strlib") if (defined $INCLIBS{"STRING"});
ADD_GLOBAL_TABLE("coroutine", "corolib") if (defined $INCLIBS{"CORO"});
ADD_GLOBAL_TABLE("table", "tablib") if (defined $INCLIBS{"TABLE"});
ADD_GLOBAL_TABLE("utf8", "utf8lib") if (defined $INCLIBS{"UTF8"});
ADD_GLOBAL_TABLE("debug", "dblib") if (defined $INCLIBS{"DEBUG"});
PROCESS_FINAL();



PROCESS_STRINGS();
