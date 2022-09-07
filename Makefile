.PHONY: all

CF=-std=c++2a -O3 -g
SRCcpp=$(wildcard src/*.cpp)
SRCh=$(wildcard src/*.h)
OBJS=$(subst .cpp,.o,$(SRCcpp))
PCH=$(subst .h,.h.gch,$(SRCh)) # precompiled headers .gch

all: dirs build/a.out
	build/a.out

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
