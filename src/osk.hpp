#pragma once

#include "../FBInk/fbink.h"
#include "_kblayout.hpp"

void osk_setup(unsigned int width, unsigned int height) {
    unsigned int blockw = width / OSK_W;
    unsigned int blockh = height / OSK_H;
    uint8_t bpp = 1u;
    float radius = 10.f;
    float spacing = 3.f;
    for (unsigned int i = 0; i < OSK_NKEYS; i++) {
        osk_keys[i].rrect.width = static_cast<unsigned int>(osk_keys[i].w * static_cast<float>(blockw));
        osk_keys[i].rrect.height = static_cast<unsigned int>(osk_keys[i].h * static_cast<float>(blockh));
        osk_keys[i].rrect.bpp = bpp;
        osk_keys[i].rrect.radius = radius;
        osk_keys[i].rrect.spacing = spacing;
        osk_keys[i].rrect.text = osk_keys[i].text;
        osk_keys[i].rrect.render();
    }
}

void osk_render(int fd, FBInkConfig * config, unsigned int osk_y, unsigned int width, unsigned int height) {
    unsigned int blockw = width / OSK_W;
    unsigned int blockh = height / OSK_H;
    short cfg_row = config->row;
    short cfg_col = config->col;
    config->row = 0;
    config->col = 0;
    // Batch it
    config->no_refresh = true;
    for (unsigned int i = 0; i < OSK_NKEYS; i++) {
        unsigned int x = static_cast<unsigned int>(osk_keys[i].x * static_cast<float>(blockw));
        unsigned int y = static_cast<unsigned int>(osk_keys[i].y * static_cast<float>(blockh));
        // printf("%d %d %d %d %d\n", i, x, y, osk_keys[i].rrect.width, osk_keys[i].rrect.height);
        fbink_print_raw_data(
                fd,
                osk_keys[i].rrect.dst,
                osk_keys[i].rrect.width,
                osk_keys[i].rrect.height,
                osk_keys[i].rrect.width * osk_keys[i].rrect.height * osk_keys[i].rrect.bpp,
                static_cast<short int>(x),
                static_cast<short int>(osk_y + y),
                config
                );
    }
    config->no_refresh = false;
    // Refresh it and make sure it won't be merged
    fbink_refresh(fd, osk_y, 0, width, height, config);
    fbink_wait_for_complete(fd, LAST_MARKER);
    config->row = cfg_row;
    config->col = cfg_col;
}

const kbkey * osk_press(unsigned int width, unsigned int height, unsigned int x, unsigned int y) {
    unsigned int blockw = width / OSK_W;
    unsigned int blockh = height / OSK_H;
    for (unsigned int i = 0; i < OSK_NKEYS; i++) {
        unsigned int kx = static_cast<unsigned int>(osk_keys[i].x * static_cast<float>(blockw));
        unsigned int ky = static_cast<unsigned int>(osk_keys[i].y * static_cast<float>(blockh));
        unsigned int kw = osk_keys[i].rrect.width;
        unsigned int kh = osk_keys[i].rrect.height;
        if (x >= kx && x < kx + kw && y >= ky && y < ky + kh) {
            return &osk_keys[i];
        }
    }
    return 0;
}
