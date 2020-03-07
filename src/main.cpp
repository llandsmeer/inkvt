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

#include "./netev.hpp"
#include "./input.hpp"
#include "./pseudotty.hpp"
#include "./vterm.hpp"
#include "./_keymap.hpp"
#include "./buffers.hpp"
#include "./rivalapp.hpp"

#ifndef GITHASH
#define GITHASH "<unknown>"
#endif

Inputs inputs;
PseudoTTY pty;
VTermToFBInk vterm;
KeycodeTranslation keytrans;
#if defined(TARGET_KOBO) && defined(EXPERIMENTAL)
// WARNING: Enabling this is VERY experimental
// I tried to use tracexec (simpler case) on my kobo
// and it failed. I think I have to rewrite some of the
// arm specific syscall handing code...
// RivalApps: Starve nickle, koreader or anything reading
// from /dev/input/event{0,1,2} from input and SIGSTOP them
// Problem: we ungrab their evdev devices, but don't know
// which ones to give back, so we grab none for them
std::vector<RivalApp> rivalapps;
#endif

void handle_atexit() {
    puts("atexit_called");
    inputs.atexit();
#if defined(TARGET_KOBO) && defined(EXPERIMENTAL)
    for (RivalApp & rivalapp : rivalapps) {
        rivalapp.give_back_control();
    }
#endif
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

int main() {
    Buffers buffers;
    pty.setup();
    vterm.setup();
    inputs.add_progout(pty.master);
    inputs.add_serial();
#ifdef INPUT_EVDEV
    inputs.add_evdev();
#else
    inputs.add_ttyraw();
#endif
    inputs.add_signals();
    inputs.add_http(7800);
    atexit(handle_atexit);
    pty.set_size(vterm.state.max_rows, vterm.state.max_cols);
    deque_printf(buffers.vt100_in, "inkvt\r\nversion %s\r\n", GITHASH);
    if (inputs.is_listening_on_http()) {
        print_listen_adresses(buffers);
    }
#if defined(TARGET_KOBO) && defined(EXPERIMENTAL)
    rivalapps = RivalApp::search();
    if (rivalapps.size() > 0) {
        for (RivalApp & rival : rivalapps) {
            deque_printf(buffers.vt100_in, "Taking over [%d] %s\r\n", rival.pid, rival.cmdline.c_str());
            rival.takeover();
        }
    }
#endif
    for (;;) {
        inputs.wait(buffers);
#ifdef INPUT_EVDEV
        while (buffers.serial.size() >= netev_size) {
            struct input_event ev;
            if (netev_read(buffers.serial.data(), ev)) {
                // remove inital netev_size items
                std::vector<char>(buffers.serial.begin()+netev_size, buffers.serial.end()).swap(buffers.serial);
                if(ev.type == EV_KEY) {
                    if (ev.value == 1 || ev.value == 2) {
                        buffers.scancodes.push_back(ev.code | 0x100);
                    } else if(ev.value == 0) {
                        buffers.scancodes.push_back(ev.code | 0);
                    }
                }
            } else {
                // magic check failed, discard some data and try again
                buffers.serial.erase(buffers.serial.begin());
            }
        }
#else
        while (buffers.serial.size() > 0) {
            int c = buffers.serial.front();
            buffers.serial.pop_front();
            buffers.keyboard.push_back(c);
        }
#endif
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
            int c = buffers.keyboard.front();
            buffers.keyboard.pop_front();
            if (keytrans.is_ctrl()) {
                c = c & 31;
            }
            pty.write(c);
        }
        while (buffers.prog_stdout.size() > 0) {
            int c = buffers.prog_stdout.front();
            buffers.prog_stdout.pop_front();
            buffers.vt100_in.push_back(c);
        }
        while (buffers.vt100_in.size() > 0) {
            int c = buffers.vt100_in.front();
            buffers.vt100_in.pop_front();
            vterm.write(c);
        }
    }
}


