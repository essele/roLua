
lee:    lee.c src/liblua.a
	gcc -o lee -Lsrc -llua -Isrc lee.c

src/liblua.a:   src/*.c src/*.h
	cd src && make
