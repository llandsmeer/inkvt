#pragma once

#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

void setup_serial(int fd) {
    // https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/#writing
    struct termios tty = {};
    if (tcgetattr(fd, &tty) == -1) {
        perror("tcgetattr");
        exit(1);
    }
    tty.c_cflag &= ~PARENB; // clear parity
    tty.c_cflag &= ~CSTOPB; // num stop bits = 1
    tty.c_cflag |= CS8; // 8 bits/byte
    tty.c_cflag &= ~CRTSCTS; // disable RTS/CTS hardware flow control
    tty.c_cflag |= CREAD | CLOCAL;
    tty.c_lflag &= ~ICANON; // non-canonical mode: dont wait for enter
    tty.c_lflag &= ~ECHO; // Disable echo
    tty.c_lflag &= ~ECHOE; // Disable erasure
    tty.c_lflag &= ~ECHONL; // Disable new-line echo
    tty.c_lflag &= ~ISIG; // no signal intepreting
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // disable any special handling of received bytes
    tty.c_oflag &= ~OPOST; // prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR; // prevent conversion of newline to carriage return/line feed
    // tty.c_oflag &= ~OXTABS; // prevent conversion of tabs to spaces (NOT PRESENT IN LINUX)
    // tty.c_oflag &= ~ONOEOT; // prevent removal of C-d chars (0x004) in output (NOT PRESENT IN LINUX)
    // tty.c_cc[VTIME] = 0;
    // tty.c_cc[VMIN] = 0;
    cfsetispeed(&tty, B9600);
    cfsetospeed(&tty, B9600);
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        exit(1);
    }
}
