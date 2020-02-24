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

    static void update_fg_color(VTermColor * c, uint8_t def) {
        if (VTERM_COLOR_IS_RGB(c)) {
            fbink_set_fg_pen_rgba(c->rgb.red, c->rgb.green, c->rgb.blue, 0xFFu, false);
        } else {
            fbink_set_fg_pen_gray(def, false);
        }
    }

    static void update_bg_color(VTermColor * c, uint8_t def) {
        if (VTERM_COLOR_IS_RGB(c)) {
            fbink_set_bg_pen_rgba(c->rgb.red, c->rgb.green, c->rgb.blue, 0xFFu, false);
        } else {
            fbink_set_bg_pen_gray(def, false);
        }
    }

    void write(char byte) {
        vterm_input_write(term, &byte, 1);
    }

    static int term_damage(VTermRect rect, void * user) {
        VTermToFBInk * me = (VTermToFBInk*)user;
        VTermScreenCell cell;
        VTermPos pos;
        int row, col;
        for (row = rect.start_row; row < rect.end_row; row++) {
            for (col = rect.start_col; col < rect.end_col; col++) {
                pos.col = col;
                pos.row = row;
                me->config.col = col;
                me->config.row = row;
                vterm_screen_get_cell(me->screen, pos, &cell);

                // NOTE: And again after the print call
                // if (cell.attrs.reverse) me->config->is_inverted = !me->config->is_inverted;

                update_fg_color(&cell.fg, 0xFFu);
                update_bg_color(&cell.bg, 0x00u);

                if (cell.chars[0] == 0U) {
                    fbink_grid_clear(me->fbfd, 1U, 1U, &me->config);
                } else {
                    fbink_print(me->fbfd, reinterpret_cast<char *>(cell.chars), &me->config);
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
        me->config.col = old.col;
        me->config.row = old.row;
        update_fg_color(&cell.fg, 0x00u);
        update_bg_color(&cell.bg, 0xFFu);
        if (cell.chars[0] == 0U) {
            fbink_grid_clear(me->fbfd, 1U, 1U, &me->config);
        } else {
            fbink_print(me->fbfd, reinterpret_cast<char *>(cell.chars), &me->config);
        }
        vterm_screen_get_cell(me->screen, pos, &cell);
        update_fg_color(&cell.fg, 0xFFu);
        update_bg_color(&cell.bg, 0x00u);
        if (cell.chars[0] == 0U) {
            fbink_grid_clear(me->fbfd, 1U, 1U, &me->config);
        } else {
            fbink_print(me->fbfd, reinterpret_cast<char *>(cell.chars), &me->config);
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
        fbink_cls(fbfd, &config, nullptr);
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
