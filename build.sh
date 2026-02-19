#!/bin/bash

set -e

INCLUDES="-I."
LIBS="-lm"

if command -v pkg-config >/dev/null 2>&1; then
	SDL2_CFLAGS="$(pkg-config --cflags sdl2 2>/dev/null || true)"
	SDL2_LIBS="$(pkg-config --libs sdl2 2>/dev/null || true)"
	if [ -n "$SDL2_CFLAGS" ] && [ -n "$SDL2_LIBS" ]; then
		INCLUDES="$INCLUDES $SDL2_CFLAGS"
		LIBS="$LIBS $SDL2_LIBS"
	fi
fi

if ! echo "$LIBS" | grep -q -- "-lSDL2"; then
	if [ -d "/opt/homebrew/include" ] && [ -d "/opt/homebrew/lib" ]; then
		INCLUDES="-I/opt/homebrew/include $INCLUDES"
		LIBS="-L/opt/homebrew/lib -lSDL2 $LIBS"
	elif [ -d "/usr/include/SDL2" ] || [ -f "/usr/include/SDL2/SDL.h" ]; then
		INCLUDES="$INCLUDES -I/usr/include/SDL2"
		LIBS="$LIBS -lSDL2"
	else
		echo "SDL2 not found. Install SDL2 dev package or ensure pkg-config can find it." >&2
		echo "On Ubuntu/Debian: sudo apt install libsdl2-dev" >&2
		echo "On macOS (Homebrew): brew install sdl2" >&2
		exit 1
	fi
fi

echo "Compiling heightgen_perlin.c..."
gcc -c heightgen_perlin.c -o heightgen_perlin.o $INCLUDES

echo "Compiling heightgen_sine.c..."
gcc -c heightgen_sine.c -o heightgen_sine.o $INCLUDES

echo "Compiling heightgen_gaussian.c..."
gcc -c heightgen_gaussian.c -o heightgen_gaussian.o $INCLUDES

echo "Compiling main.c..."
gcc -c main.c -o main.o $INCLUDES

echo "Linking..."
gcc main.o heightgen_perlin.o heightgen_sine.o heightgen_gaussian.o -o window $LIBS

echo "Build complete: ./window"
