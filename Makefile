# Compila em: bin/ping_lds

CC = g++
CFLAGS = -std=c++11 -W -Wall -Wno-unused-parameter -Wno-reorder -w
CNAMEFILE = -o ping_lds

all: clean compile_files
	

compile_files: src/sources/main.cpp
	mkdir -p bin && cd bin
	$(CC) $(CFLAGS) ../src/sources/main.cpp $(CNAMEFILE) 

clean:
	rm -rf bin/ping_lds
	cd bin/
