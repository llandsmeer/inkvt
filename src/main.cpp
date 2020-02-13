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

int main() {
#ifdef TARGET_KOBO
    system("killall nickel");
    setup_drivers();
#endif
    setup();
    inputs.setup();
    if (fbfd == -1) {
        printf("couldnt open fbink device\n");
    }
    fbink_print(fbfd, "HELLO WORLD", &config);
    config.row = 5;
    for (;;) {
        inputs.wait();
        while (inputs.keymap.messages.size() > 0) {
            int c = inputs.keymap.messages.front();
            inputs.keymap.messages.pop_front();
            if (fbfd != -1) {
                fbink_printf(fbfd, 0, &config, "%c", c);
                config.col += 1;
                if (config.col >= 80) {
                    config.row += 1;
                    config.col = 0;
                }
            } else {
                printf("%c\n", c);
            }
            if (c == 'q') break;
        }
    }
}
