#include "heightgen.h"
#include <stdlib.h>
#include <math.h>

void generate_sine(float *buffer)
{
    int num_waves = 8;
    
    float freqX[8];
    float freqY[8];
    float phase[8];
    float amp[8];
    
    for (int i = 0; i < num_waves; i++) {
        freqX[i] = ((float)rand() / RAND_MAX) * 0.05f + 0.01f;
        freqY[i] = ((float)rand() / RAND_MAX) * 0.05f + 0.01f;
        phase[i] = ((float)rand() / RAND_MAX) * 2.0f * 3.14159265f;
        amp[i] = ((float)rand() / RAND_MAX) * 7.5f + 2.5f;
    }
    
    for (int y = 0; y < HEIGHT_H; y++) {
        for (int x = 0; x < HEIGHT_W; x++) {
            float value = 0.0f;
            
            for (int i = 0; i < num_waves; i++) {
                value += sinf(x * freqX[i] + phase[i]) * 
                         sinf(y * freqY[i] + phase[i] * 0.7f) * amp[i];
            }
            
            value = (value / (float)num_waves + 1.0f) * 0.5f;
            
            buffer[y * HEIGHT_W + x] = value;
        }
    }
}
