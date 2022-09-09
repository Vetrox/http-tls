.PHONY: all

CF=-std=c++2a -O2 -g -fsanitize=address,undefined,leak -D_GLIBCXX_DEBUG -Wall -Wextra -pedantic -Wshadow -Wsign-conversion -Wconversion -Wunreachable-code -Wdisabled-optimization# -Werror
SRCcpp=$(wildcard src/*.cpp)
SRCh=$(wildcard src/*.h)
OBJS=$(SRCcpp:.cpp=.o)
PCH=$(SRCh:.h=.h.gch) # precompiled headers .gch

all: dirs build/a.out
	build/a.out

re: clean all

dirs:
	mkdir -p build

clean-objs:
	rm -f src/*.o

clean-pch:
	rm -f src/*.gch

clean: clean-objs clean-pch
	rm -rf build

build/a.out: $(OBJS) $(PCH)
	g++ $(OBJS) -o build/a.out $(CF)

src/%.h.gch: src/%.h
	g++ -x c++-header -c $< -o $@ $(CF)

src/%.o: src/%.cpp
	g++ -c $< -o $@ $(CF)
