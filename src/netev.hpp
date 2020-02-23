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

#include <stdint.h>
#include <unistd.h>
#include <endian.h> // this is bad
#include <linux/input.h>

/* We send evdev events over serial using the following structure
 *
 * struct evdev_network_event {
 *   uint32_t magic;
 *   struct {
 *       int64_t tv_sec;
 *       uint32_t tv_usec;
 *   } time;
 *   uint16_t type;
 *   uint16_t code;
 *   uint32_t value;
 * };
 *
 * all ints are sent as little endian
 */

const uint32_t NETEV_MAGIC = 0xEFDEFEF7; // EVDEV EVenT

#if BYTE_ORDER == LITTLE_ENDIAN
// my laptop and kobo ereader are little endian anyway
struct __attribute__((packed)) _netev_le {
  uint32_t magic;
  struct {
      int64_t tv_sec;
      uint32_t tv_usec;
  } time;
  uint16_t type;
  uint16_t code;
  uint32_t value;
};
#else
#error how did you end up on a big endian system?
#endif

const int netev_size = sizeof(struct _netev_le);
        //unsigned int size = read(fd, &ev, sizeof(struct input_event));

int netev_write(int fd, const struct input_event & ev) {
    struct _netev_le le;
    le.magic = NETEV_MAGIC;
    le.time.tv_sec = ev.time.tv_sec;
    le.time.tv_usec = ev.time.tv_usec;
    le.type = ev.type;
    le.code = ev.code;
    le.value = ev.value;
    return write(fd, &le, sizeof(le));
}

int netev_read(const char buf[netev_size], struct input_event & ev) {
    struct _netev_le * le = (struct _netev_le*)buf;
    if (le->magic != NETEV_MAGIC) {
        return 0;
    }
    ev.time.tv_sec = le->time.tv_sec;
    ev.time.tv_usec = le->time.tv_usec;
    ev.type = le->type;
    ev.code = le->code;
    ev.value = le->value;
    return 1;
}

int netev_read(int fd, struct input_event & ev) {
    char buf[netev_size];
    if(read(fd, buf, netev_size) == netev_size) {
        return netev_read(buf, ev);
    }
    return 0;
}
