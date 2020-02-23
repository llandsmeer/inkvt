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
#include <vector>

#include "./netev.hpp"
#include "./setup_serial.hpp"

static int _is_event_device(const struct dirent *dir) {
    return strncmp("event", dir->d_name, 5) == 0;
}

std::vector<struct pollfd> fds;

void open_evdevs() {
    struct pollfd desc;
    struct dirent **namelist;
    int ndev = scandir("/dev/input", &namelist, &_is_event_device, alphasort);
    for (int i = 0; i < ndev; i++) {
        char fname[512];
        snprintf(fname, sizeof(fname), "/dev/input/%s", namelist[i]->d_name);
        desc.fd = open(fname, O_RDONLY, O_NONBLOCK);
        if (desc.fd == -1) {
            perror("open");
            printf("couldn't open %s %d\n", fname, desc.fd);
            continue;
        }
        printf("[%d]: %s\n", desc.fd, fname);
        desc.events = POLLIN;
        fds.push_back(desc);
    }
}

int main() {
    struct input_event ev;
    int fdout = open("/dev/ttyACM0", O_WRONLY, 0);
    if (fdout == -1) {
        perror("open(/dev/ttyACM0, ..");
        return 1;
    }
    setup_serial(fdout);
    open_evdevs();
    for (;;) {
        ppoll(fds.data(), fds.size(), 0, 0);
        for (const struct pollfd & desc : fds) {
            if (!desc.revents) continue;
            int size = read(desc.fd, &ev, sizeof(struct input_event));
            if (size < (int)sizeof(struct input_event)) {
                printf("error reading from fd %d\n", desc.fd);
                continue;
            }
            if (ev.type == EV_KEY) {
                if (netev_write(fdout, ev) == -1) {
                    perror("netev_write");
                }
            }
        }
    }
}
