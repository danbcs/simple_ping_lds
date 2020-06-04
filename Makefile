# Compila todos os arquivos

CC = g++
CFLAGS = -std=c++11 -W -Wall -Wno-unused-parameter -Wno-reorder -w
CNAMEFILE = -o ping_lds

all: clean compile_files
	

compile_files: src/sources/main.cpp src/headers/main.hpp
	mkdir -p bin
	$(CC) $(CFLAGS) src/sources/main.cpp $(CNAMEFILE) 

clean:
	rm -rf ping_lds
