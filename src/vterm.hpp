#include <algorithm>

#include "../libvterm-0.1.3/include/vterm.h"
#include "../FBInk/fbink.h"


extern "C" {
    extern bool g_isQuiet;
}

class VTermToFBInk {
public:
    VTerm * term;
    VTermScreen * screen;
    VTermScreenCallbacks vtsc;

    int fbfd;
    FBInkConfig config = { 0 };
    FBInkState state = { 0 };
    FBInkDump dump = { 0 };

    void update_fg_color(VTermColor * c) {
        vterm_screen_convert_color_to_rgb(screen, c);
#define BG(x) (127 + (x)/2)
        fbink_set_bg_pen_rgba(BG(c->rgb.red), BG(c->rgb.green), BG(c->rgb.blue), 0xFFu, false, true);
#undef BG
    }

    void update_bg_color(VTermColor * c) {
        vterm_screen_convert_color_to_rgb(screen, c);
        fbink_set_fg_pen_rgba(c->rgb.red, c->rgb.green, c->rgb.blue, 0xFFu, false, true);
    }

    void output_char(const char c) {
        char buf[2] = {c, 0};
        if (c == 0) {
            fbink_grid_clear(fbfd, 1U, 1U, &config);
        } else {
            fbink_print(fbfd, buf, &config);
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

        //fprintf(stdout, "Called term_damage on (%d, %d) to (%d, %d)\n", rect.start_col, rect.start_row, rect.end_col, rect.end_row);
        // NOTE: Optimize large rects by only doing a single refresh call, instead of paired with cell-per-cell drawing.
        me->config.no_refresh = true;

        for (row = rect.start_row; row < rect.end_row; row++) {
            for (col = rect.start_col; col < rect.end_col; col++) {
                pos.col = col;
                pos.row = row;
                me->config.col = col;
                me->config.row = row;
                vterm_screen_get_cell(me->screen, pos, &cell);
                // NOTE: And again after the print call
                // if (cell.attrs.reverse) me->config->is_inverted = !me->config->is_inverted;
                me->update_fg_color(&cell.fg);
                me->update_bg_color(&cell.bg);
                me->output_char(cell.chars[0]);
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
        VTermToFBInk * me = (VTermToFBInk*)user;
        VTermScreenCell cell;

        // remove previous cursor
        vterm_screen_get_cell(me->screen, old, &cell);
        me->config.col = old.col;
        me->config.row = old.row;
        me->update_fg_color(&cell.fg);
        me->update_bg_color(&cell.bg);
        me->output_char(cell.chars[0]);

        // add new cursor
        vterm_screen_get_cell(me->screen, pos, &cell);
        me->config.col = pos.col;
        me->config.row = pos.row;
        me->update_fg_color(&cell.bg); // NOTE: fb and bg inverted
        me->update_bg_color(&cell.fg); // for color inversion
        me->output_char(cell.chars[0]);
        return 1;
    }

    static int term_moverect(VTermRect dst, VTermRect src, void * user) {
        term_damage(dst, user);
        return 1;
        // this should work I thing but doesnt:
        // preferable, a direct copy will be better
        // but something that works is also nice
        /*
        VTermToFBInk * me = (VTermToFBInk*)user;
        short int x_off, y_off;
        unsigned short int w, h;
        x_off = me->state.glyph_width*src.start_row;
        y_off = me->state.glyph_height*src.start_col;
        w = me->state.glyph_width*(src.end_row - src.start_row + 1);
        h = me->state.glyph_height*(src.end_col - src.start_col + 1);
        if (h > 100) h = 100;
        if (w > 100) w = 100;
        fbink_region_dump(me->fbfd, x_off, y_off, w, h, &me->config, &me->dump);
        me->dump.area.top = me->state.glyph_width*dst.start_row;
        me->dump.area.left = me->state.glyph_height*dst.start_col;
        fbink_restore(me->fbfd, &me->config, &me->dump);
        // fbink_free_dump_data(&me->dump);
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
        config.fontname = FONT_INDEX_E::TERMINUS;
#ifdef TARGET_KOBO
        config.fontmult = 3;
#else
        config.fontmult = 1;
#endif
        fbink_init(fbfd, &config);
        fbink_cls(fbfd, &config, nullptr);
        fbink_get_state(&config, &state);
        g_isQuiet = 1; // couldn't get config.quiet to work

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
