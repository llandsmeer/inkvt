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

#include <sys/timerfd.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <poll.h>
#include <linux/input.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/signalfd.h>

#include "setup_serial.hpp"
#include "buffers.hpp"
#include "insecure_http.hpp"
#include "vterm.hpp"

#ifndef TARGET_KOBO
static int _is_event_device(const struct dirent *dir) {
    return strncmp("event", dir->d_name, 5) == 0;
}
#endif

class Inputs {
public:
    Server server;
    bool had_input = 0;

    struct {
        int x = 0;
        int y = 0;
        int xev = 0;
        int yev = 0;
        int moved = 0;
    } istate;
private:
    enum fdtype {
        FD_EVDEV,
        FD_SERIAL,
        FD_PROGOUT,
        FD_SERVER,
        FD_SIGNAL,
        FD_STDIN,
        FD_VTERM_TIMER,
        FD_TIMER_NO_INPUT,
    };
    fdtype fdtype[128];
    struct pollfd fds[128];
    int nfds = 0;
    bool should_reset_termios = 0;
    struct termios termios_reset = { 0 };
    VTermToFBInk * vterm = 0;

    void handle_evdev(Buffers & buffers, struct input_event ev) {
        int handled = 1;
        if (ev.type == EV_ABS) {
            if ((ev.code == ABS_MT_POSITION_X || ev.code == ABS_X) && ev.value != 0) {
                if (ev.value != istate.x) istate.moved += 1;
                istate.x = ev.value;
                istate.xev = 1;
                handled = 1;
            } else if ((ev.code == ABS_MT_POSITION_Y || ev.code == ABS_Y) && ev.value != 0) {
                if (ev.value != istate.y) istate.moved += 1;
                istate.y = ev.value;
                istate.yev = 1;
                handled = 1;
            }
        } else if(false && ev.type == EV_KEY) { // maybe I should just remove old evdev keyboard handling
            if (ev.value == 1 || ev.value == 2) {
                buffers.scancodes.push_back(ev.code | 0x100);
                handled = 1;
            } else if(ev.value == 0) {
                buffers.scancodes.push_back(ev.code | 0);
                handled = 1;
            }
        }
        if (!handled) {
            printf("EVENT: %d %d %d\n", ev.type, ev.code, ev.value);
        }
    }

    void handle_evdev(Buffers & buffers, int fd) {
        struct input_event ev;
        unsigned int size = read(fd, &ev, sizeof(struct input_event));
        if (size < sizeof(struct input_event)) {
            printf("error reading from fd %d\n", fd);
            return;
        }
        handle_evdev(buffers, ev);
    }

    void handle_serial(Buffers & buffers, int fd) {
        char buf[1];
        for (;;) {
            int nread = read(fd, buf, sizeof(buf));
            if (nread == -1 || nread == 0) { // errno = EAGAIN for blocking read
                break;
            }
            for (int n = 0; n < nread; n++) {
                buffers.serial.push_back(buf[n]);
            }
        }
    }

    void handle_progout(Buffers & buffers, int fd) {
        char buf[64];
        int nread = read(fd, buf, sizeof(buf));
        if (nread < 0) return;
        for (int i = 0; i < nread; i++) {
            buffers.vt100_in.push_back(buf[i]);
        }
        // NOTE: Don't read out everything available
        // That would mean blocking in this function,
        // which disables receiving signals
        // poll() will call us again if there is more data
    }

    void handle_server(Buffers & buffers, int fd) {
        if (fd != server.fd) return;
        server.accept(buffers.keyboard);
    }

    void handle_vterm_timer(Buffers & buffers, int fd) {
        uint64_t buf;
        if (read(fd, &buf, sizeof(buf)) > 0) {
            vterm->tick();
        }
    }

    void handle_signal(Buffers & buffers, int fd) {
        struct signalfd_siginfo fdsi;
        ssize_t s = read(fd, &fdsi, sizeof(struct signalfd_siginfo));
        if (s != sizeof(fdsi)) return;
        if (fdsi.ssi_signo == SIGINT) {
            buffers.keyboard.push_back(0x03);
        } else {
            printf("Got signal %d, exiting now\n", fdsi.ssi_signo);
            exit(EXIT_SUCCESS);
        }
    }

    void handle_stdin(Buffers & buffers, int fd) {
        char c;
        while(read(fd, &c, 1) == 1) {
            buffers.keyboard.push_back(c);
        }
    }

    void handle_input_timeout(Buffers & buffers, int fd) {
        uint64_t buf;
        if (read(fd, &buf, sizeof(buf)) > 0 && !had_input) {
            printf("input timeout\n");
            exit(1);
        }
    }

public:
    void wait(Buffers & buffers) {
        poll(fds, nfds, -1);
        for (int i = 0; i < nfds; i++) {
            if (!fds[i].revents) continue;
            if (fdtype[i] == FD_EVDEV) {
                handle_evdev(buffers, fds[i].fd);
            } else if (fdtype[i] == FD_SERIAL) {
                handle_serial(buffers, fds[i].fd);
            } else if (fdtype[i] == FD_PROGOUT) {
                if (fds[i].revents & POLLHUP) {
                    // pty slave disconnected
                    exit(0);
                } else {
                    handle_progout(buffers, fds[i].fd);
                }
            } else if (fdtype[i] == FD_SERVER) {
                handle_server(buffers, fds[i].fd);
            } else if (fdtype[i] == FD_SIGNAL) {
                handle_signal(buffers, fds[i].fd);
            } else if (fdtype[i] == FD_STDIN) {
                handle_stdin(buffers, fds[i].fd);
            } else if (fdtype[i] == FD_VTERM_TIMER) {
                handle_vterm_timer(buffers, fds[i].fd);
            } else if (fdtype[i] == FD_TIMER_NO_INPUT) {
                handle_input_timeout(buffers, fds[i].fd);
            }
        }
    }

    void add_evdev() {
#ifdef TARGET_KOBO
        // Touch input is always event1 on Kobo, and we *definitely* don't want to pickup the gyro if there's one...
        int fd = open("/dev/input/event1", O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd == -1) {
            printf("couldn't open `/dev/input/event1` %d\n", fd);
            return;
        }
        printf("opened `/dev/input/event1`\n");
        fdtype[nfds] = FD_EVDEV;
        fds[nfds].events = POLLIN;
        fds[nfds++].fd = fd;
#else
        struct dirent **namelist;
        int ndev = scandir("/dev/input", &namelist, &_is_event_device, alphasort);
        for (int i = 0; i < ndev; i++) {
            char fname[512];
            snprintf(fname, sizeof(fname), "/dev/input/%s", namelist[i]->d_name);
            int fd = open(fname, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
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
            fds[nfds].events = POLLIN;
            fds[nfds++].fd = fd;
        }
#endif
    }

    void add_progout(int fd) {
        fdtype[nfds] = FD_PROGOUT;
        fds[nfds].events = POLLIN | POLLHUP;
        fds[nfds++].fd = fd;
    }

    void add_vterm_timer(int fd, VTermToFBInk * vterm) {
        fdtype[nfds] = FD_VTERM_TIMER;
        fds[nfds].events = POLLIN;
        fds[nfds++].fd = fd;
        this->vterm = vterm;
    }

    bool add_serial() {
#ifdef TARGET_KOBO
        // NOTE: Do our best to make this mostly portable...
        //       See http://trac.ak-team.com/trac/browser/niluje/Configs/trunk/Kindle/Kobo_Hacks/KoboStuff/src/usr/local/stuff/bin/usbnet-toggle.sh for a similar example.
        // NOTE: Fun fact: I don't know when Kobo started shipping g_serial, but they didn't on Mk.5, so, here's one I just built to test on my H2O:
        //       http://files.ak-team.com/niluje/mrpub/Other/USBSerial-Kobo-Mk5-H2O.tar.gz
        const char *platform = getenv("PLATFORM");

        // Abort if we can't tell the platform from the env.
        // NOTE: We could also compute that from FBInk's state via device_platform, although that's not a 1:1 mapping...
        if (platform == NULL) {
            puts("add_serial() is only supported on Kobo devices with a proper PLATFORM set in the env!");
            return false;
        }

        char module_path[PATH_MAX] = { 0 };

        snprintf(module_path, sizeof(module_path), "/drivers/%s/usb/gadget/g_serial.ko", platform);
        if (access(module_path, F_OK) != 0) {
            puts("add_serial() is only supported on Kobo devices with a g_serial kernel module!");
            return false;
        }

        // Cheap Mk. 7+ detection...
        snprintf(module_path, sizeof(module_path), "/drivers/%s/usb/gadget/configfs.ko", platform);
        if (access(module_path, F_OK) == 0) {
            // Mk. 7+
            snprintf(module_path, sizeof(module_path), "insmod /drivers/%s/usb/gadget/configfs.ko", platform);
            (void)system(module_path);
            snprintf(module_path, sizeof(module_path), "insmod /drivers/%s/usb/gadget/libcomposite.ko", platform);
            (void)system(module_path);
            snprintf(module_path, sizeof(module_path), "insmod /drivers/%s/usb/gadget/u_serial.ko", platform);
            (void)system(module_path);
            snprintf(module_path, sizeof(module_path), "insmod /drivers/%s/usb/gadget/usb_f_acm.ko", platform);
            (void)system(module_path);
            snprintf(module_path, sizeof(module_path), "insmod /drivers/%s/usb/gadget/g_serial.ko", platform);
            (void)system(module_path);
        } else {
            // Older devices
            snprintf(module_path, sizeof(module_path), "insmod /drivers/%s/usb/gadget/arcotg_udc.ko", platform);
            (void)system(module_path);
            snprintf(module_path, sizeof(module_path), "insmod /drivers/%s/usb/gadget/g_serial.ko", platform);
            (void)system(module_path);
        }

        // Sleep a bit to leave the kernel time to breathe, because everything is terrible
        const struct timespec zzz   = { 0L, 500000000L };
        nanosleep(&zzz, NULL);

        int fd = open("/dev/ttyGS0", O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd != -1) {
            fdtype[nfds] = FD_SERIAL;
            fds[nfds].events = POLLIN;
            fds[nfds++].fd = fd;
            puts("opening /dev/ttyGS0");
            setup_serial(fd);
            return true;
        } else {
            printf("couldn't open /dev/ttyGS0: %m\n");
            return false;
        }
#else
        puts("add_serial() is only supported on Kobo devices");
#endif
        return false;
    }

    int add_http(int port) {
        if (server.setup(port) < -1) {
            return -1;
        }
        fdtype[nfds] = FD_SERVER;
        fds[nfds++] = server.get_pollfd();
        return 0;
    }

    bool is_listening_on_http() {
        return server.fd != -1;
    }

    void add_ttyraw() {
        should_reset_termios = 1;
        tcgetattr(STDIN_FILENO, &termios_reset);
        struct termios raw = termios_reset;
        raw.c_lflag &= ~(ECHO | ICANON);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
        fdtype[nfds] = FD_STDIN;
        fds[nfds].events = POLLIN;
        fds[nfds].revents = 0;
        fds[nfds++].fd = STDIN_FILENO;
    }

    void add_signals(std::vector<int> signals) {
        sigset_t mask = { 0 };
        sigemptyset(&mask);
        for (int signal : signals) {
            if (sigaddset(&mask, signal) != 0) {
                puts("sigaddset");
                exit(1);
            }
        }
        if (sigprocmask(SIG_BLOCK, &mask, 0) == -1) {
            perror("sigprocmask");
            exit(1);
        }
        int fd = signalfd(-1, &mask, 0);
        if (fd == -1) {
            perror("signalfd");
            exit(1);
        }
        fdtype[nfds] = FD_SIGNAL;
        fds[nfds].events = POLLIN;
        fds[nfds].revents = 0;
        fds[nfds++].fd = fd;
    }

    void add_signals() {
        add_signals({SIGINT, SIGQUIT});
    }

    void atexit() {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios_reset);
    }

    void add_exit_after(int seconds) {
        itimerspec ts;
        int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
        if (timerfd < 0) {
            perror("add_exit_after:timerfd_create");
            exit(1);
        }
        ts.it_value.tv_sec = seconds;
        ts.it_value.tv_nsec = 0;
        ts.it_interval.tv_sec = 0;
        ts.it_interval.tv_nsec = 0;
        long err = timerfd_settime(timerfd, 0, &ts, 0);
        if (err < 0) {
            perror("add_exit_after:timerfd_settime");
            exit(1);
        }
        fdtype[nfds] = FD_TIMER_NO_INPUT;
        fds[nfds].events = POLLIN;
        fds[nfds++].fd = timerfd;
    }

};
