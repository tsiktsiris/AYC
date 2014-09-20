CC=mpic++
OPTIMIZATION=-O3
CFLAGS=-ansi -std=c++0x  -pedantic -Iincludes $(OPTIMIZATION)
LDFLAGS=-g -O3
EXEC=run
SRC=$(wildcard src/*.cpp)
OBJ=$(SRC:src/%.cpp=obj/%.o)



all:$(EXEC)

default:all

run:$(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

obj/%.o:src/%.cpp
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -f obj/*.o* $(EXEC) *.zip debug
