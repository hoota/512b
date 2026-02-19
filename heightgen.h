#ifndef HEIGHTGEN_H
#define HEIGHTGEN_H

#define HEIGHT_W 1200
#define HEIGHT_H 401
#define HEIGHT_SIZE (HEIGHT_W * HEIGHT_H)

void generate_perlin(float *buffer);
void generate_sine(float *buffer);
void generate_gaussian(float *buffer);

#endif
