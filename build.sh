#!/bin/bash

set -e

INCLUDES="-I/opt/homebrew/include -I."
LIBS="-L/opt/homebrew/lib -lSDL2 -lm"

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
