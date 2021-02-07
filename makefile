make:
	gcc src/*.c -Wall -Wextra -pedantic-errors -lSDL2 -lSDL2_ttf -o nes
run:
	./nes
