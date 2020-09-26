#pragma once

#include "../FBInk/fbink.h"
#include "_kblayout.hpp"

void osk_setup(int width, int height) {
    int blockw = width / OSK_W;
    int blockh = height / OSK_H;
    int bpp = 1;
    float radius = 10;
    float spacing = 10;
    for (int i = 0; i < OSK_NKEYS; i++) {
        osk_keys[i].rrect.width = osk_keys[i].w * blockw;
        osk_keys[i].rrect.height = osk_keys[i].h * blockh;
        osk_keys[i].rrect.bpp = bpp;
        osk_keys[i].rrect.radius = radius;
        osk_keys[i].rrect.spacing = spacing;
        osk_keys[i].rrect.render();
    }
}

void osk_render(int fd, FBInkConfig * config, int osk_y, int width, int height) {
    int blockw = width / OSK_W;
    int blockh = height / OSK_H;
    short cfg_row = config->row;
    short cfg_col = config->col;
    config->row = 0;
    config->col = 0;
    for (int i = 0; i < OSK_NKEYS; i++) {
        int x = osk_keys[i].x * blockw;
        int y  = osk_keys[i].y * blockh;
        // printf("%d %d %d %d %d\n", i, x, y, osk_keys[i].rrect.width, osk_keys[i].rrect.height);
        fbink_print_raw_data(
                fd,
                osk_keys[i].rrect.dst,
                osk_keys[i].rrect.width,
                osk_keys[i].rrect.height,
                osk_keys[i].rrect.width * osk_keys[i].rrect.height * osk_keys[i].rrect.bpp,
                x,
                osk_y + y,
                config
                );
    }
    config->row = cfg_row;
    config->col = cfg_col;
}
