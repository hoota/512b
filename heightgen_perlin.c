#include "heightgen.h"
#include <stdlib.h>
#include <math.h>

static int perm[512];
static float gradX[256];
static float gradY[256];

static void init_perlin(void)
{
    for (int i = 0; i < 256; i++) {
        perm[i] = i;
        float angle = (float)i * 2.0f * 3.14159265f / 256.0f;
        gradX[i] = cosf(angle);
        gradY[i] = sinf(angle);
    }
    for (int i = 255; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = perm[i];
        perm[i] = perm[j];
        perm[j] = tmp;
    }
    for (int i = 0; i < 256; i++) {
        perm[256 + i] = perm[i];
    }
}

static float fade(float t)
{
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

static float lerp(float a, float b, float t)
{
    return a + t * (b - a);
}

static float grad(int hash, float x, float y)
{
    int h = hash & 255;
    return gradX[h] * x + gradY[h] * y;
}

static float noise2d(float x, float y)
{
    int xi = (int)floorf(x) & 255;
    int yi = (int)floorf(y) & 255;
    
    float xf = x - floorf(x);
    float yf = y - floorf(y);
    
    float u = fade(xf);
    float v = fade(yf);
    
    int aa = perm[perm[xi] + yi];
    int ab = perm[perm[xi] + yi + 1];
    int ba = perm[perm[xi + 1] + yi];
    int bb = perm[perm[xi + 1] + yi + 1];
    
    float x1 = lerp(grad(aa, xf, yf), grad(ba, xf - 1, yf), u);
    float x2 = lerp(grad(ab, xf, yf - 1), grad(bb, xf - 1, yf - 1), u);
    
    return (lerp(x1, x2, v) + 1.0f) * 0.5f;
}

void generate_perlin(float *buffer)
{
    init_perlin();
    
    float frequency = 0.02f;
    int octaves = 4;
    float amplitude = 2.0f;
    float persistence = 0.5f;
    
    for (int y = 0; y < HEIGHT_H; y++) {
        for (int x = 0; x < HEIGHT_W; x++) {
            float value = 0.0f;
            float amp = amplitude;
            float freq = frequency;
            
            for (int o = 0; o < octaves; o++) {
                value += noise2d(x * freq, y * freq) * amp;
                amp *= persistence;
                freq *= 2.0f;
            }
            
            buffer[y * HEIGHT_W + x] = value;
        }
    }
}
