#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "heightgen.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define V 0.02f
#define T_INCREMENT 0.016f
#define PIXEL_W 1200
#define PIXEL_H 400
#define PIXEL_SIZE (PIXEL_W * PIXEL_H)

typedef struct {
    float x, y, z;
    float r, g, b;
    float phase;
    float time_phase;
    float freq_x, freq_y, freq_z;
    float offset_x, offset_y;
} Light;

typedef struct {
    float nx, ny, nz;
} Normal;

static float height_buffer[HEIGHT_SIZE];
static Normal normal_buffer[PIXEL_SIZE];
static float T = 0.0f;

static int utf8_decode(const char **s)
{
    int k = **(unsigned char **)s;
    if (k < 0x80) {
        (*s)++;
        return k;
    }
    int mask = 0x40;
    int count = 0;
    while (k & mask) { mask >>= 1; count++; }
    int codepoint = k & (mask - 1);
    (*s)++;
    while (count--) {
        codepoint = (codepoint << 6) | (**(unsigned char **)s & 0x3F);
        (*s)++;
    }
    return codepoint;
}

static void box_blur(float *buffer, int w, int h, int radius, int passes)
{
    float *temp = malloc(w * h * sizeof(float));
    
    for (int p = 0; p < passes; p++) {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                float sum = 0.0f;
                int count = 0;
                
                for (int dy = -radius; dy <= radius; dy++) {
                    for (int dx = -radius; dx <= radius; dx++) {
                        int nx = x + dx;
                        int ny = y + dy;
                        if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                            sum += buffer[ny * w + nx];
                            count++;
                        }
                    }
                }
                temp[y * w + x] = sum / count;
            }
        }
        for (int i = 0; i < w * h; i++) {
            buffer[i] = temp[i];
        }
    }
    
    free(temp);
}

static void render_text_to_height(float *buffer, const char *text, float depth)
{
    const char *font_path = "fonts/Philosopher-Regular.ttf";
    float font_size = 200.0f;
    
    FILE *f = fopen(font_path, "rb");
    if (!f) {
        fprintf(stderr, "Cannot open font: %s\n", font_path);
        return;
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    unsigned char *font_data = malloc(size);
    fread(font_data, 1, size, f);
    fclose(f);
    
    stbtt_fontinfo font;
    if (!stbtt_InitFont(&font, font_data, 0)) {
        fprintf(stderr, "Failed to init font\n");
        free(font_data);
        return;
    }
    
    float scale = stbtt_ScaleForPixelHeight(&font, font_size);
    
    int text_width = 0;
    int x_pos = 0;
    
    const char *p = text;
    while (*p) {
        int codepoint = utf8_decode(&p);
        int ax, lsb;
        stbtt_GetCodepointHMetrics(&font, codepoint, &ax, &lsb);
        text_width += ax * scale;
    }
    
    int min_y = 0, max_y = 0;
    p = text;
    while (*p) {
        int codepoint = utf8_decode(&p);
        int y0, y1;
        stbtt_GetCodepointBitmapBox(&font, codepoint, scale, scale, NULL, &y0, NULL, &y1);
        if (y0 < min_y) min_y = y0;
        if (y1 > max_y) max_y = y1;
    }
    
    int full_height = max_y - min_y;
    int start_x = (PIXEL_W - text_width) / 2;
    int start_y = (PIXEL_H - full_height) / 2 - min_y;
    
    unsigned char *text_bitmap = calloc(PIXEL_W * PIXEL_H, 1);
    
    x_pos = start_x;
    p = text;
    while (*p) {
        int codepoint = utf8_decode(&p);
        int w, h, xo, yo;
        unsigned char *bitmap = stbtt_GetCodepointBitmap(&font, scale, scale, codepoint, &w, &h, &xo, &yo);
        
        int px = x_pos + xo;
        int py = start_y + yo;
        
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                int tx = px + x;
                int ty = py + y;
                if (tx >= 0 && tx < PIXEL_W && ty >= 0 && ty < PIXEL_H) {
                    int idx = ty * PIXEL_W + tx;
                    int existing = text_bitmap[idx];
                    int val = bitmap[y * w + x];
                    if (val > existing) text_bitmap[idx] = val;
                }
            }
        }
        
        free(bitmap);
        
        int ax, lsb;
        stbtt_GetCodepointHMetrics(&font, codepoint, &ax, &lsb);
        x_pos += ax * scale;
    }
    
    float *text_height_buffer = calloc(PIXEL_W * PIXEL_H, sizeof(float));
    for (int i = 0; i < PIXEL_W * PIXEL_H; i++) {
        text_height_buffer[i] = text_bitmap[i] / 255.0f;
    }
    
    box_blur(text_height_buffer, PIXEL_W, PIXEL_H, 3, 2);
    
    for (int y = 0; y < PIXEL_H; y++) {
        for (int x = 0; x < PIXEL_W; x++) {
            int idx = y * HEIGHT_W + x;
            int pidx = y * PIXEL_W + x;
            height_buffer[idx] += text_height_buffer[pidx] * depth;
        }
    }
    
    free(text_bitmap);
    free(text_height_buffer);
    free(font_data);
}

static void compute_all_normals(void)
{
    for (int y = 0; y < PIXEL_H; y++) {
        for (int x = 0; x < PIXEL_W; x++) {
            float h = height_buffer[y * HEIGHT_W + x];
            float hx = height_buffer[y * HEIGHT_W + x + 1];
            float hy = height_buffer[(y + 1) * HEIGHT_W + x];
            
            float dx = hx - h;
            float dy = hy - h;
            
            float nx = -dx;
            float ny = -dy;
            float nz = 1.0f;
            
            float len = sqrtf(nx * nx + ny * ny + nz * nz);
            normal_buffer[y * PIXEL_W + x].nx = nx / len;
            normal_buffer[y * PIXEL_W + x].ny = ny / len;
            normal_buffer[y * PIXEL_W + x].nz = nz / len;
        }
    }
}

static void update_light_position(Light *light)
{
    float speed = V * (1.0f + sinf(light->time_phase + T));
    light->phase += speed;
    
    float center_x = (float)(PIXEL_W / 2);
    float center_y = (float)(PIXEL_H / 2);
    float amplitude_x = (float)(PIXEL_W / 2 - 50);
    float amplitude_y = (float)(PIXEL_H / 2 - 50);
    
    light->x = center_x + amplitude_x * sinf(light->phase * light->freq_x + light->offset_x);
    light->y = center_y + amplitude_y * sinf(light->phase * light->freq_y + light->offset_y);
    light->z = 100.0f + 50.0f * sinf(light->phase * light->freq_z);
}

static void render_frame(uint32_t *pixels, int pitch, Light *lights)
{
    for (int y = 0; y < PIXEL_H; y++) {
        for (int x = 0; x < PIXEL_W; x++) {
            Normal *n = &normal_buffer[y * PIXEL_W + x];
            
            float fr = 0.0f, fg = 0.0f, fb = 0.0f;
            
            for (int i = 0; i < 3; i++) {
                float lx = lights[i].x - (float)x;
                float ly = lights[i].y - (float)y;
                float lz = lights[i].z;
                
                float len_l = sqrtf(lx * lx + ly * ly + lz * lz);
                float cos = (n->nx * lx + n->ny * ly + n->nz * lz) / len_l;
                
                if (cos < 0.0f) cos = 0.0f;
                
                fr += cos * lights[i].r;
                fg += cos * lights[i].g;
                fb += cos * lights[i].b;
            }
            
            if (fr > 1.0f) fr = 1.0f;
            if (fg > 1.0f) fg = 1.0f;
            if (fb > 1.0f) fb = 1.0f;
            
            uint8_t r = (uint8_t)(fr * 255.0f);
            uint8_t g = (uint8_t)(fg * 255.0f);
            uint8_t b = (uint8_t)(fb * 255.0f);
            
            pixels[y * pitch + x] = (255 << 24) | (b << 16) | (g << 8) | r;
        }
    }
}

int main(void)
{
    SDL_Init(SDL_INIT_VIDEO);
    
    SDL_Window *window = SDL_CreateWindow(
        "Foil Reflection",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1200, 400,
        0
    );
    
    generate_perlin(height_buffer);
    render_text_to_height(height_buffer, "перестань", 5.0f);
    compute_all_normals();
    
    Light lights[3];
    
    lights[0].phase = 0.0f;
    lights[0].time_phase = 0.0f;
    lights[0].freq_x = 1.0f; lights[0].freq_y = 1.3f; lights[0].freq_z = 0.7f;
    lights[0].offset_x = 0.0f; lights[0].offset_y = 0.0f;
    lights[0].r = 1.0f; lights[0].g = 0.0f; lights[0].b = 0.0f;
    
    lights[1].phase = 0.0f;
    lights[1].time_phase = 2.094f;
    lights[1].freq_x = 0.8f; lights[1].freq_y = 1.1f; lights[1].freq_z = 0.9f;
    lights[1].offset_x = 1.047f; lights[1].offset_y = 2.094f;
    lights[1].r = 0.0f; lights[1].g = 1.0f; lights[1].b = 0.0f;
    
    lights[2].phase = 0.0f;
    lights[2].time_phase = 4.189f;
    lights[2].freq_x = 1.2f; lights[2].freq_y = 0.9f; lights[2].freq_z = 1.1f;
    lights[2].offset_x = 2.094f; lights[2].offset_y = 4.189f;
    lights[2].r = 0.0f; lights[2].g = 0.0f; lights[2].b = 1.0f;
    
    for (int i = 0; i < 3; i++) {
        update_light_position(&lights[i]);
    }
    
    SDL_Surface *surface = SDL_GetWindowSurface(window);
    
    SDL_Event event;
    int running = 1;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
        }
        
        for (int i = 0; i < 3; i++) {
            update_light_position(&lights[i]);
        }
        
        SDL_LockSurface(surface);
        uint32_t *pixels = (uint32_t *)surface->pixels;
        int pitch = surface->pitch / 4;
        render_frame(pixels, pitch, lights);
        SDL_UnlockSurface(surface);
        
        SDL_UpdateWindowSurface(window);
        
        T += T_INCREMENT;
        SDL_Delay(16);
    }
    
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}
