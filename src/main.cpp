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
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>

#include "./input.hpp"
#include "./pseudotty.hpp"
#include "./vterm.hpp"
#include "./_keymap.hpp"
#include "./buffers.hpp"
#include "./osk.hpp"

#include "../cxxopts/include/cxxopts.hpp"

#ifndef GITHASH
#define GITHASH "<unknown>"
#endif

Inputs inputs;
PseudoTTY pty;
VTermToFBInk vterm;
KeycodeTranslation keytrans;

void handle_atexit() {
    puts("atexit_called");
    inputs.atexit();
}

void deque_printf(std::deque<char> & out, const char * fmt, ...) {
    char result[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(result, sizeof(result), fmt, ap);
    for (char * c = result; *c; c++) {
        out.push_back(*c);
    }
    va_end(ap);
}

void print_listen_adresses(Buffers & buffers) {
    struct ifaddrs *ifaddr, *ifa;
    int s;
    char host[NI_MAXHOST];
    deque_printf(buffers.vt100_in, "listening on:\r\n");
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        deque_printf(buffers.vt100_in, "    ... could not call getifaddres()\r\n");
        return;
    }
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr)
            continue;
        s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
        if (s == 0 && ifa->ifa_addr->sa_family == AF_INET) {
            deque_printf(buffers.vt100_in, "  - %s:%d (%s)\r\n",
                    host, inputs.server.port, ifa->ifa_name);
        }
    }
    freeifaddrs(ifaddr);
}

int main(int argc, char ** argv) {
    cxxopts::Options arg_options("inkvt", "VT100 terminal for E-ink devices");
    arg_options.add_options()
        ("h,help", "Print usage")
        ("no-reinit", "Do not issue fbink_reinit() calls (assume no plato/nickel running)", cxxopts::value<bool>()->default_value("false"))
        ("serial", "Load g_serial and listen on serial (might break usbms until reboot)", cxxopts::value<bool>()->default_value("false"))
        ("no-http", "Do not listen on http", cxxopts::value<bool>()->default_value("false"))
        ("no-timeout", "Do not exit after 20 seconds of no input", cxxopts::value<bool>()->default_value("false"))
        ("no-signals", "Do not catch signals", cxxopts::value<bool>()->default_value("false"))
        ("osk", "Experimental OSK", cxxopts::value<bool>()->default_value("false"))
        ("f,fontname", "FBInk Bitmap fontname, one of ibm, unscii, unscii_alt, unscii_thin, unscii_fantasy, unscii_mcr, unscii_tall, block, leggie, veggie, kates, fkp, ctrld, orp, orpb, orpi, scientifica, scientificab, scientificai, terminus, terminusb, fatty, spleen, tewi, tewib, topaz, microknight, vga or cozette",
            cxxopts::value<std::string>()->default_value("terminus"))
        ("s,fontsize", "Fontsize multiplier", cxxopts::value<int>()->default_value("2"))
        ("d,debug", "Enable debug", cxxopts::value<bool>()->default_value("false"))
        ("c,shell", "Shell (full path)", cxxopts::value<std::string>()->default_value("/bin/sh"))
        ("i,input", "Initial stdin line (eg, call init script)", cxxopts::value<std::string>()->default_value(""))
    ;
    auto arg_result = arg_options.parse(argc, argv);
    if (arg_result.count("help")) {
        std::cout << arg_options.help() << std::endl;
        exit(0);
    }
    Buffers buffers;
    std::string shell = arg_result["shell"].as<std::string>();
    pty.setup(shell.c_str());
    std::string init_stdin_line = arg_result["input"].as<std::string>();
    if (init_stdin_line.length() > 0) {
        deque_printf(buffers.keyboard, "%s\r\n", init_stdin_line.c_str());
    }
    std::string fontname = arg_result["fontname"].as<std::string>();
    vterm.has_osk = arg_result["osk"].as<bool>();
    if (vterm.has_osk) {
        inputs.add_evdev();
    }
    bool debug = arg_result["debug"].as<bool>();
    vterm.setup(arg_result["fontsize"].as<int>(), fontname.c_str());
    bool reinit_on_damage = false;
    if (!arg_result["no-reinit"].as<bool>()) {
        reinit_on_damage = true;
    }
    inputs.add_progout(pty.master);
    if (arg_result["serial"].as<bool>()) {
        if (inputs.add_serial()) {
            deque_printf(buffers.vt100_in, "reading from serial\r\n", GITHASH);
        }
    }
    inputs.add_ttyraw();
    if (!arg_result["no-signals"].as<bool>()) {
        inputs.add_signals();
    }
    if (!arg_result["no-http"].as<bool>()) {
        if (inputs.add_http(7800) < 0) {
            deque_printf(buffers.vt100_in, "http server setup failed\r\n", GITHASH);
        }
    }
    inputs.add_vterm_timer(vterm.timerfd, &vterm);
    atexit(handle_atexit);
    pty.set_size(vterm.nrows(), vterm.ncols());
    deque_printf(buffers.vt100_in, "inkvt\r\nversion %s\r\n", GITHASH);
    if (inputs.is_listening_on_http()) {
        print_listen_adresses(buffers);
    }
    if (!arg_result["no-timeout"].as<bool>()) {
        int seconds = 20;
        inputs.add_exit_after(seconds);
        deque_printf(buffers.vt100_in, "waiting %d seconds on input\r\n", seconds);
    }
    if (debug) {
        deque_printf(buffers.vt100_in,
        "FBInk %s\r\nview %ux%u dev %s/%s/%s/%hu offset %hhu/%hhu/%hhu rota %hhu/%hhu/%d/%hhu/%d\r\n",
            fbink_version(),
            vterm.state.view_width,
            vterm.state.view_height,
            vterm.state.device_name,
            vterm.state.device_codename,
            vterm.state.device_platform,
            vterm.state.device_id,
            vterm.state.view_hori_origin,
            vterm.state.view_vert_origin,
            vterm.state.view_vert_offset,
            vterm.state.ntx_boot_rota,
            vterm.state.ntx_rota_quirk,
            vterm.state.is_ntx_quirky_landscape,
            vterm.state.current_rota,
            vterm.state.can_rotate
        );
    }
    for (;;) {
        inputs.wait(buffers);
        if (reinit_on_damage) {
            if (vterm.reinit()) {
                pty.set_size(vterm.nrows(), vterm.ncols());
            }
        }
        if (vterm.has_osk) {
            if (inputs.istate.xev && inputs.istate.yev) {
                inputs.istate.xev = 0;
                inputs.istate.yev = 0;
                inputs.istate.moved = 0;
                int x;
                int y;
#ifdef TARGET_KOBO
                // On Kobo, the touch panel has a fixed rotation, one that *never* matches the actual rotation.
                // Handle the initial translation here so that it makes sense @ (canonical) UR...
                // (This is generally a -90°/+90°, made trickier because there's a layout swap so height/width are swapped).
                // c.f., rotate_touch_coordinates in FBInk for a different, possibly less compatible approach...

                // Speaking of, handle said layout shenanigans now...
                int dim_swap;
                if ((fbink_rota_native_to_canonical(vterm.state.current_rota) & 1u) == 0) {
                    // Canonical rotation is even (UR/UD)
                    dim_swap = vterm.state.screen_width;
                } else {
                    // Canonical rotation is odd (CW/CCW)
                    dim_swap = vterm.state.screen_height;
                }

                // And the various extra device-specific quirks on top of that...
                // c.f., https://github.com/koreader/koreader/blob/master/frontend/device/kobo/device.lua
                if (vterm.state.device_id == 310 || vterm.state.device_id == 320) {
                    // Touch A/B & Touch C. This will most likely be wrong for one of those.
                    // touch_mirrored_x
                    x = dim_swap - inputs.istate.x;
                    y = inputs.istate.y;
                } else if (vterm.state.device_id == 374) {
                    // Aura H2O²r1
                    // touch_switch_xy
                    x = inputs.istate.y;
                    y = inputs.istate.x;
                } else {
                    // touch_switch_xy && touch_mirrored_x
                    x = dim_swap - inputs.istate.y;
                    y = inputs.istate.x;
                }
                if (debug) {
                    printf("input touch @ (%d, %d) -> (%d, %d)\n", inputs.istate.x, inputs.istate.y, x, y);
                }
#else
                x = inputs.istate.x;
                y = inputs.istate.y;
                if (debug) {
                    printf("input touch @ (%d, %d)\n", x, y);
                }
#endif
                const char * kb = vterm.click(x, y, debug);
                for (unsigned i = 0; i < strlen(kb); i++) {
                    buffers.keyboard.push_back(kb[i]);
                }
            }
        }
        while (buffers.serial.size() > 0) {
            int c = buffers.serial.front();
            buffers.serial.pop_front();
            buffers.keyboard.push_back(c);
        }
        while (buffers.scancodes.size() > 0) {
            int c = buffers.scancodes.front();
            buffers.scancodes.pop_front();
            if (c >> 8) {
                // key press
                keytrans.press(c & 0xff, buffers.keyboard);
            } else {
                // key release
                keytrans.release(c & 0xff, buffers.keyboard);
            }
        }
        while (buffers.keyboard.size() > 0) {
            inputs.had_input = 1;
            int c = buffers.keyboard.front();
            buffers.keyboard.pop_front();
            if (keytrans.is_ctrl()) {
                c = c & 31;
            }
            pty.write(c);
        }
        while (buffers.vt100_in.size() > 0) {
            int c = buffers.vt100_in.front();
            buffers.vt100_in.pop_front();
            vterm.write(c);
        }
    }
}


