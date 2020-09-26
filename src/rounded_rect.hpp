#pragma once

#include <cstdlib>
#include <cassert>
#include <cstdint>

class RoundedRect {
    static int _abs(int a) {
        return a > 0 ? a : -a;
    }
    static int _clamp(int a) {
        return a > 0 ? a : 0;
    }
public:
    uint8_t * dst = 0;
    int bpp = 1; // BPP: Y YA RGB RGBA
    int width = 30;
    int height = 30;
    float radius = 6;
    float spacing = 10;
    uint8_t color = 128;
    uint8_t alpha = 128; // for even bpp

    void render() {
        int len = width * height * bpp;
        dst = (uint8_t*)realloc(dst, len);
        float mx = width / 2., my = height / 2.;
        int bpp = len / (width * height);
        int a = 0;
        int b = 0;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float dx = _clamp(_abs(mx - x) - (width - spacing)/2. + radius);
                float dy = _clamp(_abs(my - y) - (height - spacing)/2. + radius);
                bool inside = dx*dx + dy*dy < radius;
                int idx = (y*width + x)*bpp;
                if (inside) a++;
                else b++;
                for (int p = 0; p < (bpp & ~1); p++) {
                    dst[idx+p] = inside ? color : 255;
                }
                if ((bpp & 1) == 0) {
                    dst[idx+bpp-1] = alpha;
                }
            }
        }
    }
};
