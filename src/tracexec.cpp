#include "tracexec.hpp"

int main (int argc, const char ** argv){
    long err;
    pid_t pid = atoi(argv[1]);
    tracexec tracee;
    tracee.pid = pid;
    kill(pid, SIGSTOP);
    try_ptrace(PTRACE_SEIZE, pid, 0, 0);
    waitpid(pid, 0, 0);
    try_ptrace(PTRACE_GETREGS, pid, 0, &tracee.reset_regs);
    int fd = 0, flags;
    while ((flags = tracee.fcntl(fd, F_GETFL, 0)) >= 0) {
        printf("[%d]: %d\n", fd, flags);
        fd += 1;
    }
    try_ptrace(PTRACE_SETREGS, pid, 0, &tracee.reset_regs);
    try_ptrace(PTRACE_DETACH, pid, 0, SIGCONT);
}
