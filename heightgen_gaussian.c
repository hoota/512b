#include "heightgen.h"
#include <stdlib.h>
#include <math.h>

void generate_gaussian(float *buffer)
{
    int num_hills = 15;
    
    float cx[15];
    float cy[15];
    float sigma[15];
    float height[15];
    
    for (int i = 0; i < num_hills; i++) {
        cx[i] = ((float)rand() / RAND_MAX) * HEIGHT_W;
        cy[i] = ((float)rand() / RAND_MAX) * (HEIGHT_H - 1);
        sigma[i] = ((float)rand() / RAND_MAX) * 80.0f + 30.0f;
        height[i] = ((float)rand() / RAND_MAX) * 8.0f + 2.0f;
    }
    
    for (int y = 0; y < HEIGHT_H; y++) {
        for (int x = 0; x < HEIGHT_W; x++) {
            float value = 0.0f;
            
            for (int i = 0; i < num_hills; i++) {
                float dx = x - cx[i];
                float dy = y - cy[i];
                float dist2 = dx * dx + dy * dy;
                float s2 = sigma[i] * sigma[i];
                value += height[i] * expf(-dist2 / (2.0f * s2));
            }
            
            buffer[y * HEIGHT_W + x] = value;
        }
    }
}
