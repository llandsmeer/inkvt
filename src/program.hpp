#pragma once

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <deque>
#include <signal.h>
#include <sys/wait.h>

class Program {
    int killed = false;
public:
    int fd_in;
    int fd_out;
    pid_t pid;

    void write(const char * text) {
        ::write(fd_in, text, strlen(text));
    }

    void write_char(char c) {
        if (::write(fd_in, &c, 1) == -1) {
            perror("Program::write_char:write");
        }
    }

    int read_char() {
        char buf[1];
        if (read(fd_out, buf, 1) != 1) {
            return -1;
        }
        return buf[0];
    }

    void read_all(std::deque<int> & target) {
        int c;
        while ((c = read_char()) != -1) {
            target.push_back(c);
        }
    }

    void kill() {
        if (killed) return;
        ::kill(pid, SIGKILL);
        waitpid(pid, 0, 0);
        killed = true;
    }

    void setup() {
        int pipefd_stdin[2];
        int pipefd_stdout[2];
        if (pipe(pipefd_stdin) == -1) {
            perror("pipe");
            exit(2);
        }
        if (pipe(pipefd_stdout) == -1) {
            perror("pipe");
            exit(2);
        }
        if (fcntl(pipefd_stdout[0], F_SETFL, O_NONBLOCK) < 0) {
            perror("fcntl");
            exit(2);
        }
        pid_t pid = fork();
        if (pid == -1) {
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // child
            close(pipefd_stdin[1]); // close write end
            close(pipefd_stdout[0]); // close read end
            dup2(pipefd_stdin[0], STDIN_FILENO);
            dup2(pipefd_stdout[1], STDOUT_FILENO);
            dup2(pipefd_stdout[1], STDERR_FILENO);
            close(pipefd_stdin[0]);
            close(pipefd_stdout[1]);
            char path[] = "/bin/sh";
            char arg0[] = "sh";
            char arg1[] = "-i";
            char * const argv[] = {arg0, arg1, 0};
            execv(path, argv);
            perror("execv");
            exit(EXIT_FAILURE);
        } else {
            // parent
            this->pid = pid;
            close(pipefd_stdin[0]);
            close(pipefd_stdout[1]);
            fd_in = pipefd_stdin[1];
            fd_out = pipefd_stdout[0];
        }
    }
};
