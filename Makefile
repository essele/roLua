
lee:    lee.c src/liblua.a ro_main.h ro_baselib.h
	gcc -o lee -Lsrc -llua -Isrc lee.c

src/liblua.a:   src/*.c src/*.h ro_main.h ro_mathlib.h ro_baselib.h
	cd src && make liblua.a

ro_main.h ro_baselib.h ro_mathlib.h:  src/llex.c src/ltm.c src/lbaselib.c build_rostrings.pl build_lib.pl
	(cd src && make clean) && ./build_rostrings.pl
