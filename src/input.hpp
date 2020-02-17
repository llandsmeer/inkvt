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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <poll.h>
#include <linux/input.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "./keymap.hpp"
#include "./buffers.hpp"

static int _is_event_device(const struct dirent *dir) {
    return strncmp("event", dir->d_name, 5) == 0;
}

class Inputs {
public:
    Keymap keymap;
private:
    const int FD_EVDEV = 1;
    const int FD_SERIAL = 2;
    const int FD_PROGOUT = 3;
    int fdtype[64];
    struct pollfd fds[64];
    int nfds = 0;

    struct {
        int x;
        int y;
        int moved;
    } istate;

    void _setup() {
        struct dirent **namelist;
        int ndev = scandir("/dev/input", &namelist, &_is_event_device, alphasort);
        for (int i = 0; i < ndev; i++) {
            char fname[512];
            snprintf(fname, sizeof(fname), "/dev/input/%s", namelist[i]->d_name);
            int fd = open(fname, O_RDONLY, O_NONBLOCK);
            if (fd == -1) {
                printf("couldn't open %s %d\n", fname, fd);
                continue;
            }
            if (0 && !ioctl(fd, EVIOCGRAB, (void*)1)) {
                close(fd);
                continue;
            }
            printf("opened %s\n", fname);
            fdtype[nfds] = FD_EVDEV;
            fds[nfds++].fd = fd;
        }
        int fd = open("/dev/ttyGS0", O_RDONLY | O_NONBLOCK);
        if (fd != -1) {
            fdtype[nfds] = FD_SERIAL;
            fds[nfds++].fd = fd;
        } else {
            printf("couldn't open /dev/ttyGS0\n");
        }
        for (int i = 0; i < nfds; i++) {
            fds[i].events = POLLIN;
        }
    }

    void handle_evdev(Buffers & buffers, int fd) {
        struct input_event ev;
        unsigned int size = read(fd, &ev, sizeof(struct input_event));
        int handled = 1;
        if (size < sizeof(struct input_event)) {
            printf("error reading from fd %d\n", fd);
            return;
        }
        if (ev.type == EV_ABS) {
            if (ev.code == ABS_MT_POSITION_X && ev.value != 0) {
                if (ev.value != istate.x) istate.moved = 1;
                istate.x = ev.value;
                handled = 1;
            } else if (ev.code == ABS_MT_POSITION_Y && ev.value != 0) {
                if (ev.value != istate.y) istate.moved = 1;
                istate.y = ev.value;
                handled = 1;
            }
        } else if(ev.type == EV_KEY) {
            // cat /usr/include/linux/input-event-codes.h | grep KEY_
            if (ev.value == 1 || ev.value == 2) {
                keymap.press(buffers.keyboard_in, ev.code);
                handled = 1;
            } else if(ev.value == 0) {
                keymap.release(ev.code);
                handled = 1;
            }
        }
        if (!handled) {
            printf("EVENT: %d %d %d\n", ev.type, ev.code, ev.value);
        }
    }

    void handle_serial(Buffers & buffers, int fd) {
        char buf[512];
        for (;;) {
            int nread = read(fd, buf, sizeof(buf));
            if (nread == -1) {
                // errno = EAGAIN for blocking read
                break;
            }
            for (int n = 0; n < nread; n++) {
                buffers.keyboard_in.push_back(buf[n]);
            }
        }
    }

    void handle_progout(Buffers & buffers, int fd) {
        char c;
        while (read(fd, &c, 1) == 1) {
            buffers.prog_stdout.push_back(c);
        }
    }
public:
    void setup() {
        _setup();
    }
    void wait(Buffers & buffers) {
        ppoll(fds, nfds, 0, 0);
        for (int i = 0; i < nfds; i++) {
            if (!fds[i].revents) continue;
            if (fdtype[i] == FD_EVDEV) {
                handle_evdev(buffers, fds[i].fd);
            }
            if (fdtype[i] == FD_SERIAL) {
                handle_serial(buffers, fds[i].fd);
            }
            if (fdtype[i] == FD_PROGOUT) {
                handle_progout(buffers, fds[i].fd);
            }
        }
    }
    void add_progout(int fd) {
        fdtype[nfds] = FD_PROGOUT;
        fds[nfds].events = POLLIN;
        fds[nfds++].fd = fd;
    }
};
