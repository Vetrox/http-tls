.PHONY: all

CF=-std=c++2a -O3 -g

all: dirs pool build/a.out
	build/a.out

dirs:
	mkdir -p build

clean-cache:
	rm -f src/*.o

clean: clean-cache
	rm -rf build 

build/a.out: pool
	g++ $(subst .cpp,.o,$(wildcard src/*.cpp)) -o build/a.out $(CF) 

pool: $(subst .cpp,.o,$(wildcard src/*.cpp))

src/%.o: src/%.cpp
	g++ -c $< -o $@ $(CF)
