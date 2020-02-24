#include <algorithm>

#include "../libvterm-0.1.3/include/vterm.h"
#include "../FBInk/fbink.h"

class VTermToFBInk {
public:
    VTerm * term;
    VTermScreen * screen;
    VTermScreenCallbacks vtsc;

    int fbfd;
    FBInkConfig config = { 0 };
    FBInkState state = { 0 };

    static uint8_t brightness(VTermColor * c, uint8_t def) {
        if (VTERM_COLOR_IS_RGB(c)) {
            return 0.2989*c->rgb.red + 0.5870*c->rgb.green + 0.1140*c->rgb.blue;
        }
        return def;
    }

    void write(char byte) {
        vterm_input_write(term, &byte, 1);
    }

    static int term_damage(VTermRect rect, void * user) {
        VTermToFBInk * me = (VTermToFBInk*)user;
        VTermScreenCell cell;
        VTermPos pos;
        uint8_t fg, bg;
        int row, col;
        for (row = rect.start_row; row < rect.end_row; row++) {
            for (col = rect.start_col; col < rect.end_col; col++) {
                pos.col = col;
                pos.row = row;
                me->config.col = col;
                me->config.row = row;
                vterm_screen_get_cell(me->screen, pos, &cell);

                fg = brightness(&cell.fg, 255);
                bg = brightness(&cell.bg, 0);

                // if (cell.attrs.reverse) std::swap(fg, bg);

                me->config.fg_color = fg;
                me->config.bg_color = bg;

                if (!*cell.chars) {
                    fbink_grid_clear(me->fbfd, 1U, 1U, &me->config);
                } else {
                    fbink_print(me->fbfd, *cell.chars, &me->config);
                }

            }
        }
        return 1;
    }

    static int term_movecursor(VTermPos pos, VTermPos old, int visible, void * user) {
        return 1;
        VTermToFBInk * me = (VTermToFBInk*)user;
        VTermScreenCell cell;
        vterm_screen_get_cell(me->screen, old, &cell);
        me->config.fg_color = brightness(&cell.fg, 255);
        me->config.bg_color = brightness(&cell.bg, 0);
        if (!*cell.chars) {
            fbink_grid_clear(me->fbfd, 1U, 1U, &me->config);
        } else {
            fbink_print(me->fbfd, *cell.chars, &me->config);
        }
        vterm_screen_get_cell(me->screen, pos, &cell);
        me->config.fg_color = brightness(&cell.bg, 0);
        me->config.bg_color = brightness(&cell.fg, 255);
        if (!*cell.chars) {
            fbink_grid_clear(me->fbfd, 1U, 1U, &me->config);
        } else {
            fbink_print(me->fbfd, *cell.chars, &me->config);
        }
        return 1;
    }

    static int term_moverect(VTermRect dest, VTermRect src, void * user) {
        // term_damage(dest, user);
        return 1;
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
        fbink_init(fbfd, &config);
        config.bg_color = 255;
        fbink_cls(fbfd, &config, nullptr);
        fbink_state_dump(&config);
        fbink_get_state(&config, &state);

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
        screen = vterm_obtain_screen(term);
        vterm_screen_set_callbacks(screen, &vtsc, this);
        vterm_screen_enable_altscreen(screen, 1);
        vterm_screen_reset(screen, 1);
    }
};
