
lee:    lee.c src/liblua.a ros.h
	gcc -o lee -Lsrc -llua -Isrc lee.c

src/liblua.a:   src/*.c src/*.h
	cd src && make liblua.a

ros.h:  src/llex.c src/ltm.c build_rostrings.pl
	./build_rostrings.pl > ros.h
