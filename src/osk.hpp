#pragma once

#include "../FBInk/fbink.h"
#include "_kblayout.hpp"

void osk_setup(int width, int height) {
    int blockw = width / OSK_W;
    int blockh = height / OSK_H;
    int bpp = 1;
    float radius = 10;
    float spacing = 3;
    for (int i = 0; i < OSK_NKEYS; i++) {
        osk_keys[i].rrect.width = osk_keys[i].w * blockw;
        osk_keys[i].rrect.height = osk_keys[i].h * blockh;
        osk_keys[i].rrect.bpp = bpp;
        osk_keys[i].rrect.radius = radius;
        osk_keys[i].rrect.spacing = spacing;
        osk_keys[i].rrect.text = osk_keys[i].text;
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
    // Batch it
    config->no_refresh = true;
    for (int i = 0; i < OSK_NKEYS; i++) {
        int x = osk_keys[i].x * blockw;
        int y = osk_keys[i].y * blockh;
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
    config->no_refresh = false;
    // Refresh it and make sure it won't be merged
    fbink_refresh(fd, osk_y, 0, width, height, config);
    fbink_wait_for_complete(fd, LAST_MARKER);
    printf("fbink_refresh\n");
    config->row = cfg_row;
    config->col = cfg_col;
}

const kbkey * osk_press(int width, int height, int x, int y) {
    int blockw = width / OSK_W;
    int blockh = height / OSK_H;
    for (int i = 0; i < OSK_NKEYS; i++) {
        int kx = osk_keys[i].x * blockw;
        int ky = osk_keys[i].y * blockh;
        int kw = osk_keys[i].rrect.width;
        int kh = osk_keys[i].rrect.height;
        if (x >= kx && x < kx + kw && y >= ky && y < ky + kh) {
            return &osk_keys[i];
        }
    }
    return 0;
}
