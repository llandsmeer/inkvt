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
    uint8_t * dst = 0;
    int bpp = 1; // BPP: Y YA RGB RGBA
    int width = 30;
    int height = 30;
    float radius = 6;
    float spacing = 10;
    uint8_t color = 128;
    uint8_t text_color = 0;
    uint8_t alpha = 255; // for even bpp
    const char * text = "?";

    void render() {
        int len = width * height * bpp;
        dst = (uint8_t*)realloc(dst, len);
        float mx = width / 2., my = height / 2.;
        int bpp = len / (width * height);
        // DRAW ROUNDED RECT
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float dx = _clamp(_abs(mx - x) - (width - spacing)/2. + radius);
                float dy = _clamp(_abs(my - y) - (height - spacing)/2. + radius);
                bool inside = dx*dx + dy*dy < radius;
                int idx = (y*width + x)*bpp;
                // for uneven bpp, its either Y or RGB
                // for even bpp, the last component is alpha
                for (int p = 0; p < bpp; p++) { // could subtract 1 from even bpp
                    dst[idx+p] = inside ? color : 255;
                }
                if ((bpp & 1) == 0) {
                    dst[idx+bpp-1] = alpha;
                }
            }
        }
        // DRAW LABEL
        int glyphWidth = 8;
        int spacing = 2;
        int glyphHeight = 16;
        int n = strlen(text);
        int x0 = (width - ((glyphWidth+spacing) * n)) / 2 + spacing/2;
        int y0 = (height - glyphHeight) / 2;
        if (x0 < 0 || y0 < 0) return; // just skip drawing if the osk is too small
        for (int i = 0; i < n; i++) {
            int ch = text[i];
            // const unsigned char * bitmap = microknight_block1[ch];
            const unsigned char * bitmap = topaz_block1[ch];
            for (int dx = 0; dx < glyphWidth; dx++) {
                for (int dy = 0; dy < glyphHeight; dy++) {
                    bool set = bitmap[dy] & (1 << dx);
                    int x = x0 + i*(glyphWidth+spacing) + dx;
                    int y = y0 + dy;
                    int idx = (y*width + x)*bpp;
                    if (set) {
                        for (int p = 0; p < bpp; p++) { // could subtract 1 from even bpp
                            dst[idx+p] = text_color;
                        }
                    }
                }
            }
        }
    }
};
