#include <deque>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <pty.h>
#include <termios.h>
#include <fcntl.h>

#include <sys/select.h>
#include <sys/wait.h>

extern char **environ;

class PseudoTTY {
public:
    pid_t pid;
    int master;
    struct termios tios;

    void write(const char * text) {
        int nwritten = ::write(master, text, strlen(text));
        (void)nwritten;
    }

    void write(char c) {
        int nwritten = ::write(master, &c, 1);
        (void)nwritten;
    }

    void set_size(int rows, int cols) {
        struct {
            unsigned short ws_row;
            unsigned short ws_col;
            unsigned short ws_xpixel;   /* unused */
            unsigned short ws_ypixel;   /* unused */
        } winsize = {};
        winsize.ws_row = static_cast<unsigned short>(rows);
        winsize.ws_col = static_cast<unsigned short>(cols);
        ioctl(master, TIOCSWINSZ, &winsize);
    }

    void setup(const char * shell) {
        pid = forkpty(&master, 0, 0, 0);
        if (pid < 0) {
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // shell is const char *, but args is char * const
            char * non_const_shell = (char*)malloc(strlen(shell)+1);
            memcpy(non_const_shell, shell, strlen(shell)+1);
            char * const args[] = { non_const_shell, 0 };
            execve(shell, args, environ);
        } else {
            tcgetattr(master, &tios);
            // tios.c_lflag &= ~(ECHO | ECHONL);
            tcsetattr(master, TCSAFLUSH, &tios);
        }
        int flags = fcntl(master, F_GETFL, 0);
        fcntl(master, F_SETFL, flags | O_NONBLOCK);
    }
};
