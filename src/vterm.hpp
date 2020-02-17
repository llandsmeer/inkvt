#include "../libvterm-0.1.3/include/vterm.h"
#define restrict __restrict__
extern "C" {
#include "./fbink.h"
}

class VTermToFBInk {
public:
    VTerm * term;
    VTermScreen * screen;
    VTermScreenCallbacks vtsc;

    int fbfd;
    FBInkConfig config = { 0 };
    FBInkState state = { 0 };

    void write(char byte) {
        vterm_input_write(term, &byte, 1);
    }

    static int term_damage(VTermRect rect, void * user) {
        VTermToFBInk * me = (VTermToFBInk*)user;
        VTermScreenCell cell;
        VTermPos pos;
        uint8_t fg, bg, color;
        int row, col;
        for (row = rect.start_row; row < rect.end_row; row++) {
            for (col = rect.start_col; col < rect.end_col; col++) {
                pos.col = col;
                pos.row = row;
                me->config.col = col;
                me->config.row = row;
                vterm_screen_get_cell(me->screen, pos, &cell);

                //fg = rgb2vga(cell.fg.red, cell.fg.green, cell.fg.blue);
                //bg = rgb2vga(cell.bg.red, cell.bg.green, cell.bg.blue);
                //if (cell.attrs.reverse) color = bg | (fg << 4); else color = fg | (bg << 4);

                if (cell.chars[0] == 0) {
                    fbink_printf(me->fbfd, 0, &me->config, " ");
                } else {
                    fbink_printf(me->fbfd, 0, &me->config, "%c", cell.chars[0]);
                }

            }
        }
        return 1;
    }

    static int term_movecursor(VTermPos pos, VTermPos old, int visible, void * user) {
        return 1;
    }

    static int term_moverect(VTermRect dest, VTermRect src, void * user) {
        return 1;
    }

    static int term_settermprop(VTermProp prop, VTermValue * val, void * user) {
        return 1;
    }

    static int term_bell(void* user) {
        return 0;
    }

    void setup() {
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
        term = vterm_new(25, 25);
        screen = vterm_obtain_screen(term);
        vterm_screen_set_callbacks(screen, &vtsc, this);
        vterm_screen_enable_altscreen(screen, 1);
        vterm_screen_reset(screen, 1);

        fbfd = fbink_open();
        if (fbfd == -1) {
            puts("fbink_open()");
            exit(1);
        }
        fbink_init(fbfd, &config);
        fbink_cls(fbfd, &config, 0);
        fbink_state_dump(&config);
        fbink_get_state(&config, &state);
    }
};
