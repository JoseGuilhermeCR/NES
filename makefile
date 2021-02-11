make:
	g++ src/*.cc -std=c++2a -Wall -Wextra -pedantic-errors -lSDL2 -o nes
run:
	./nes
