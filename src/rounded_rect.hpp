#pragma once

#include <cstdlib>
#include <cassert>
#include <cstdint>
#include <string.h>


// a new level of cutting corners short and dependending on FBInk :)
// font options: microknight, topaz
// #include "../FBInk/fonts/microknight.h"
#include "../FBInk/fonts/topaz.h"

class RoundedRect {
    static int _abs(int a) {
        return a > 0 ? a : -a;
    }
    static int _clamp(int a) {
        return a > 0 ? a : 0;
    }
public:
    uint8_t * dst = nullptr;
    uint8_t bpp = 1u; // BPP: Y YA RGB RGBA
    unsigned int width = 30u;
    unsigned int height = 30u;
    float radius = 6.f;
    float spacing = 10.f;
    uint8_t color = 200u;
    uint8_t text_color = 0u;
    uint8_t alpha = 255u; // for even bpp
    const char * text = "?";

    void render() {
        size_t len = width * height * bpp;
        dst = (uint8_t*)realloc(dst, len);
        float mx = static_cast<float>(width) / 2.f;
        float my = static_cast<float>(height) / 2.f;
        // DRAW ROUNDED RECT
        for (unsigned int y = 0; y < height; y++) {
            for (unsigned int x = 0; x < width; x++) {
                float dx = _clamp(_abs(mx - static_cast<float>(x)) - (static_cast<float>(width) - spacing)/2.f + radius);
                float dy = _clamp(_abs(my - static_cast<float>(y)) - (static_cast<float>(height) - spacing)/2.f + radius);
                bool inside = dx*dx + dy*dy < radius;
                size_t idx = (y*width + x)*bpp;
                // for uneven bpp, its either Y or RGB
                // for even bpp, the last component is alpha
                for (unsigned int p = 0; p < bpp; p++) { // could subtract 1 from even bpp
                    dst[idx+p] = inside ? color : 255u;
                }
                if ((bpp & 1) == 0) {
                    dst[idx+bpp-1] = alpha;
                }
            }
        }
        // DRAW LABEL
        uint8_t glyphWidth = 8;
        uint8_t rspacing = 2;
        uint8_t glyphHeight = 16;
        size_t n = strlen(text);
        int x0 = (width - ((glyphWidth+rspacing) * n)) / 2u + rspacing/2u;
        int y0 = (height - glyphHeight) / 2u;
        if (x0 < 0 || y0 < 0) return; // just skip drawing if the osk is too small
        for (size_t i = 0; i < n; i++) {
            unsigned char ch = text[i];
            // const unsigned char * bitmap = microknight_block1[ch];
            const unsigned char * bitmap = topaz_block1[ch];
            for (unsigned int dx = 0; dx < glyphWidth; dx++) {
                for (unsigned int dy = 0; dy < glyphHeight; dy++) {
                    bool set = bitmap[dy] & (1 << dx);
                    unsigned int x = x0 + i*(glyphWidth+rspacing) + dx;
                    unsigned int y = y0 + dy;
                    size_t idx = (y*width + x)*bpp;
                    if (set) {
                        for (unsigned int p = 0; p < bpp; p++) { // could subtract 1 from even bpp
                            dst[idx+p] = text_color;
                        }
                    }
                }
            }
        }
    }
};
