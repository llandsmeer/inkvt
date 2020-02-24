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
#include "./pseudotty.hpp"
#include "./vterm.hpp"

#ifndef GITHASH
#define GITHASH "<unknown>"
#endif

Inputs inputs;
PseudoTTY pty;
VTermToFBInk vterm;

static void setup_drivers() {
#ifdef TARGET_KOBO
    system("insmod /drivers/mx6sll-ntx/usb/gadget/configfs.ko");
    system("insmod /drivers/mx6sll-ntx/usb/gadget/libcomposite.ko");
    system("insmod /drivers/mx6sll-ntx/usb/gadget/u_serial.ko");
    system("insmod /drivers/mx6sll-ntx/usb/gadget/usb_f_acm.ko");
    system("insmod /drivers/mx6sll-ntx/usb/gadget/g_serial.ko");
#endif
}

int main() {
    setup_drivers();
    Buffers buffers;
    pty.setup();
    vterm.setup();
    inputs.setup();
    inputs.add_progout(pty.master);
    const char header[] = "inkvt\nversion " GITHASH "\n\n";
    for (size_t i = 0; i < sizeof(header); i++) {
        buffers.vt100_in.push_back(header[i]);
    }
    for (;;) {
        inputs.wait(buffers);
        while (buffers.keyboard_in.size() > 0) {
            int c = buffers.keyboard_in.front();
            buffers.keyboard_in.pop_front();
            pty.write(c);
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
            vterm.write(c);
        }
    }
}


