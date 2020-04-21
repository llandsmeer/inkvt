/* inkvt - VT100 terminal for E-ink devices
 * Copyright (C) 2020 Lennart Landsmeer <lennart@landsmeer.email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <algorithm>
#include <sys/timerfd.h>
#include <iostream>

#include "../libvterm-0.1.3/include/vterm.h"
#include "../FBInk/fbink.h"

// reset high_throughput_mode check every <n> ms
constexpr int INTERVAL_MS = 100;
// on my laptop, throughput on high output programs
// is ~ 300k output_char() calls per 100ms
// on the kobo, random values between 4k and 14k
#ifdef TARGET_KOBO
constexpr long HIGH_THROUGHPUT_THRESHOLD = 3000;
#else
constexpr long HIGH_THROUGHPUT_THRESHOLD = 100000;
#endif
// after <n> consecutive ticks of the timer without writes
// disable timer (sleep) mode, to save battery life
constexpr int TIMER_SLEEP_MODE_THRESHOLD = 10;

class VTermToFBInk {
public:
    VTerm * term;
    VTermScreen * screen;
    VTermScreenCallbacks vtsc;

    bool reinit_on_damage = false;

    // timer to detect huge output streams
    int timerfd = -1;
    long nwrites_in_interval = 0;
    bool high_throughput_mode = false;
    VTermPos last_cursor;
    bool timer_is_running = false;
    int nticks_without_output = 0;

    itimerspec ts_on;
    itimerspec ts_off = {};
    //ts.it_value.tv_nsec = 100*1000000;
    //ts.it_interval.tv_nsec = 100*1000000;

    int fbfd;
    FBInkConfig config = { 0 };
    FBInkState state = { 0 };
    FBInkDump dump = { 0 };

    void tick() {
        if (high_throughput_mode && nwrites_in_interval < HIGH_THROUGHPUT_THRESHOLD) {
            high_throughput_mode = false;
            VTermRect full_refresh = { 0, 0, 0, 0};
            full_refresh.end_col = state.max_cols;
            full_refresh.end_row = state.max_rows;
            term_damage(full_refresh, this);
        }
        if (nwrites_in_interval == 0) {
            nticks_without_output += 1;
            if (nticks_without_output > TIMER_SLEEP_MODE_THRESHOLD) {
                timerfd_settime(timerfd, 0, &ts_off, 0);
                timer_is_running = false;
            }
        } else {
            nticks_without_output = 0;
        }
        nwrites_in_interval = 0;
    }

    void update_fg_color(VTermColor * c) {
        vterm_screen_convert_color_to_rgb(screen, c);
#define FG(x) ((255-x) / 2)
#define BG(x) (255-x)
        fbink_set_fg_pen_rgba(FG(c->rgb.red), FG(c->rgb.green), FG(c->rgb.blue), 0xFFu, false, true);
    }

    void update_bg_color(VTermColor * c) {
        vterm_screen_convert_color_to_rgb(screen, c);
        fbink_set_bg_pen_rgba(BG(c->rgb.red), BG(c->rgb.green), BG(c->rgb.blue), 0xFFu, false, true);
#undef BG
#undef FG
    }

    void output_char(const VTermPos & pos) {
        // high throughput stuff
        nwrites_in_interval += 1;
        if (timer_is_running) {
            if (high_throughput_mode) return;
            if (nwrites_in_interval > HIGH_THROUGHPUT_THRESHOLD) {
                high_throughput_mode = true;
            }
        } else if (timerfd != -1 /* we get called before timerfd creation in setup */) {
            if (timerfd_settime(timerfd, 0, &ts_on, 0) >= 0) {
                timer_is_running = true;
                nticks_without_output = 0;
            }
        }
        // drawing stuff
        VTermScreenCell cell;
        vterm_screen_get_cell(screen, pos, &cell);
        config.col = pos.col;
        config.row = pos.row;
        if (pos.row == last_cursor.row && pos.col == last_cursor.col) {
            update_fg_color(&cell.bg);
            update_bg_color(&cell.fg);
        } else {
            update_fg_color(&cell.fg);
            update_bg_color(&cell.bg);
        }
        VTermRect rect;
        rect.start_row = pos.row;
        rect.start_col = pos.col;
        rect.end_col = pos.col + 1;
        rect.end_row = pos.row + 1;
        char buf[32];
        size_t nread = vterm_screen_get_text(screen, buf, sizeof(buf)-1, rect);
        buf[nread] = 0;
        if (nread == 0) {
            fbink_grid_clear(fbfd, 1U, 1U, &config);
        } else {
            fbink_print(fbfd, buf, &config);
        }
    }

    void write(char byte) {
        vterm_input_write(term, &byte, 1);
    }

    static int term_damage(VTermRect rect, void * user) {
        VTermToFBInk * me = static_cast<VTermToFBInk*>(user);
        VTermScreenCell cell;
        VTermPos pos;
        int row, col;

        if (me->reinit_on_damage) {
            int res = fbink_reinit(me->fbfd, &me->config);
            if ((res == OK_BPP_CHANGE) | (res == OK_ROTA_CHANGE)) {
                /* if both changed, OK_BPP_CHANGE `wins' */
                fbink_get_state(&me->config, &me->state);
                printf("fbink_reinit()\n");
            }
        }

        //fprintf(stdout, "Called term_damage on (%d, %d) to (%d, %d)\n", rect.start_col, rect.start_row, rect.end_col, rect.end_row);
        // NOTE: Optimize large rects by only doing a single refresh call, instead of paired with cell-per-cell drawing.
        me->config.no_refresh = true;

        for (row = rect.start_row; row < rect.end_row; row++) {
            for (col = rect.start_col; col < rect.end_col; col++) {
                pos.col = col;
                pos.row = row;
                // NOTE: And again after the print call
                // if (cell.attrs.reverse) me->config->is_inverted = !me->config->is_inverted;
                me->output_char(pos);
            }
        }

        // Refresh the full rectangle
        me->config.no_refresh = false;
        me->config.col = rect.start_col;
        me->config.row = rect.start_row;
        fbink_grid_refresh(me->fbfd, rect.end_col - rect.start_col, rect.end_row - rect.start_row, &me->config);

        return 1;
    }

    static int term_movecursor(VTermPos pos, VTermPos old, int visible, void * user) {
        VTermToFBInk * me = static_cast<VTermToFBInk*>(user);
        me->last_cursor = pos; // keep track of cursor in high_throughput_mode
        if (me->high_throughput_mode) return 1;
        VTermScreenCell cell;
        me->output_char(old); // remove previous cursor
        me->output_char(pos); // add new cursor
        return 1;
    }

    static int term_moverect(VTermRect dst, VTermRect src, void * user) {
        term_damage(dst, user);
        return 1;
        /*
        // 'more effcient memcpy implementation':
        // This work sort of but is very buggy. Especially because the linux
        // console still likes to overwrite some parts of the screen...
        VTermToFBInk * me = (VTermToFBInk*)user;
        unsigned short int w, h;
        w = me->state.glyph_width*(src.end_col - src.start_col);
        h = me->state.glyph_height*(src.end_row - src.start_row);
        me->config.col = src.start_col;
        me->config.row = src.start_row;
        fbink_region_dump(me->fbfd, 0, 0, w, h, &me->config, &me->dump);
        me->config.col = dst.start_col;
        me->config.row = dst.start_row;
        fbink_print_raw_data(me->fbfd, me->dump.data,
                w, h, me->dump.size, 0, 0, &me->config);
        return 1;
        */
    }

    static int term_settermprop(VTermProp prop, VTermValue * val, void * user) {
        return 1;
    }

    static int term_bell(void* user) {
        return 0;
    }

    void setup() {
        fbfd = fbink_open();
        if (fbfd == -1) {
            puts("fbink_open()");
            exit(1);
        }
        // TODO: Making the font & size configurable might be nice ;).
        config.fontname = FONT_INDEX_E::TERMINUS;
#ifdef TARGET_KOBO
        config.fontmult = 2;
#else
        config.fontmult = 1;
#endif
        fbink_init(fbfd, &config);
        fbink_cls(fbfd, &config, nullptr);
        fbink_get_state(&config, &state);
        config.is_quiet = 1;
        fbink_update_verbosity(&config);

        vtsc = (VTermScreenCallbacks){
            .damage = VTermToFBInk::term_damage,
            .moverect = VTermToFBInk::term_moverect,
            .movecursor = VTermToFBInk::term_movecursor,
            .settermprop = VTermToFBInk::term_settermprop,
            .bell = VTermToFBInk::term_bell,
            .resize = 0,
            .sb_pushline = 0,
            .sb_popline = 0
        };
        term = vterm_new(state.max_rows, state.max_cols);
        vterm_set_utf8(term, 1);
        screen = vterm_obtain_screen(term);
        vterm_screen_set_callbacks(screen, &vtsc, this);
        vterm_screen_enable_altscreen(screen, 1);
        vterm_screen_reset(screen, 1);

        timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
        if (timerfd == -1) {
            perror("timerfd_create");
            exit(1);
        }
        ts_on.it_value.tv_nsec = INTERVAL_MS*1000000;
        ts_on.it_interval.tv_nsec = INTERVAL_MS*1000000;
        timerfd_settime(timerfd, 0, &ts_off, 0);
    }
};
