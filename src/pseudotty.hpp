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
        ::write(master, text, strlen(text));
    }

    void write(char c) {
        ::write(master, &c, 1);
    }

    void setup() {
        pid = forkpty(&master, 0, 0, 0);
        if (pid < 0) {
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            char shell[] = "/bin/sh";
            char * const args[] = { shell, 0 };
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
