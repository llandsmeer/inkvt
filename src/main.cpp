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

#include <stdlib.h>

#include "./input.hpp"
#include "./program.hpp"
//#include "./flush_inputstream.hpp"

#define restrict __restrict__
extern "C" {
#include "./fbink.h"
}

Inputs inputs;

int fbfd;
FBInkConfig config = { 0 };
FBInkState state = { 0 };

static void setup() {
    fbfd = fbink_open();
    if (fbfd == -1) return;
    fbink_init(fbfd, &config);
    fbink_cls(fbfd, &config, 0);
    fbink_state_dump(&config);
    fbink_get_state(&config, &state);
}

static void setup_drivers() {
    system("insmod /drivers/mx6sll-ntx/usb/gadget/configfs.ko");
    system("insmod /drivers/mx6sll-ntx/usb/gadget/libcomposite.ko");
    system("insmod /drivers/mx6sll-ntx/usb/gadget/usb_f_acm.ko");
    system("insmod /drivers/mx6sll-ntx/usb/gadget/u_serial.ko");
    system("insmod /drivers/mx6sll-ntx/usb/gadget/g_serial.ko");
}

Program program;

int main() {
#ifdef TARGET_KOBO
    system("killall nickel");
    setup_drivers();
#endif
    setup();
    Buffers buffers;
    program.setup();
    inputs.setup();
    inputs.add_progout(program.fd_out);
    if (fbfd == -1) {
        printf("couldnt open fbink device\n");
    }
    fbink_print(fbfd, "HELLO WORLD", &config);
    config.row = 5;
    for (;;) {
        //flush_inputstream(STDIN_FILENO);
        inputs.wait(buffers);
        while (buffers.keyboard_in.size() > 0) {
            int c = buffers.keyboard_in.front();
            buffers.keyboard_in.pop_front();
            program.write_char(c);
            buffers.vt100_in.push_back(c);
        }
        while (buffers.prog_stdout.size() > 0) {
            int c = buffers.prog_stdout.front();
            buffers.prog_stdout.pop_front();
            buffers.vt100_in.push_back(c);
            if (c == 'q') break;
        }
        while (buffers.vt100_in.size() > 0) {
            int c = buffers.vt100_in.front();
            buffers.vt100_in.pop_front();
            if (fbfd == -1) {
                printf("%c\n", c);
            } else if (c == '\n') {
                config.col = 0;
                config.row += 1;
            } else {
                fbink_printf(fbfd, 0, &config, "%c", c);
                config.col += 1;
                if (config.col >= state.max_cols) {
                    config.row += 1;
                    config.col = 0;
                }
            }
        }
    }
    program.kill();
}


