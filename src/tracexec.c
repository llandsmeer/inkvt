/* tracexec.c - Execute instruction in tracee
 *
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

/* gcc ./tracee_exec.c -Wall -masm=intel -falign-labels=8 -g

usage:  build/tracexec.x86 PID CMD fds [CMD2 fds2...]

cmds:
        flush     make fd temporary nonblocking
                  and perform reads of 24 bytes
                  until read() returns with an error
                  (most likely EAGAIN)
        grab      ioctl(fd, EVIOCGRAB, 1)
        ungrab    ioctl(fd, EVIOCGRAB, 0)

example:
        build/tracexec.x86 1234 flush 0 3 ungrab 3
        will flush stdin and fd 3 for pid 1234
        as well as releasing the evdev grab on fd 3
*/

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>

#include <sys/ioctl.h>
#include <sys/ptrace.h>
#include <sys/signal.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/user.h>

#include <linux/input.h>

#define try_ptrace(req, pid, addr, data) err = ptrace((req), (pid), (addr), (data)); \
        if (err == -1 && errno != 0) { \
            perror("ptrace(" #req ", ..."); \
            printf("line %d", __LINE__); \
            exit(1); \
        }

#define CONCAT(a, b) a ## b

// prepend try_ptrace(PTRACE_GETREGS, pid, 0, &regs);!
// to reset regs to initial state before exec
#define TRACEE_BEGIN(lbl) execute(&&CONCAT(lbl_code_begin, lbl), &&CONCAT(lbl_code_end, lbl), -1); \
    if (no_dead_code) { \
        goto CONCAT(lbl_code_end, lbl); \
    } \
    CONCAT(lbl_code_begin, lbl):

#define TRACEE_END(lbl) asm("int3"); \
        CONCAT(lbl_code_end, lbl):;

#define TRACEE_ASM(code) TRACEE_BEGIN(__LINE__) \
                asm(code); \
            TRACEE_END(__LINE__)

#define TRACEE_ASMN(ninstr, code) execute(&&CONCAT(lbl_code_begin, __LINE__), &&CONCAT(lbl_code_end, __LINE__), ninstr); \
    if (no_dead_code) { \
        goto CONCAT(lbl_code_end, __LINE__); \
    } \
    CONCAT(lbl_code_begin, __LINE__): \
        asm(code); \
    CONCAT(lbl_code_end, __LINE__):;

#define ALIGN_UP(addr, align) (((addr)+align-1) & ~(align-1))
#define ALIGN_DOWN(addr, align) ((addr) & ~(align-1))

#define try(x) (err = (x), (err < 0 ? \
        (printf("ERROR: " #x " = %ld\n", err), exit(1)) : 0), err)

pid_t pid;
volatile int no_dead_code = 1;
long tracee_syscall(long nr, long narg, ...);
void execute(const char * begin, const char * end, int ninstr);

#ifdef __amd64__
struct user_regs_struct reset_regs;
struct user_regs_struct regs;
#define IP(regs) regs.rip
#define SP(regs) regs.rsp
#endif

#ifdef __arm__
struct user_regs reset_regs;
struct user_regs regs;
#define IP(regs) regs.uregs[15]
#define SP(regs) regs.uregs[13]
#endif

const int MODE_FLUSH = 1;
const int MODE_EVIOCGRAB = 2;
const int MODE_EVIOCUNGRAB = 3;

int main(int argc, char ** argv) {
    long err;
    if (argc < 3) {
        printf("usage:  %s PID CMD fds [CMD2 fds2...]\n\n", argv[0]);
        printf("cmds:\n");
        printf("        flush     make fd temporary nonblocking\n");
        printf("                  and perform reads of %ld bytes\n", (long)sizeof(struct input_event));
        printf("                  until read() returns with an error\n");
        printf("                  (most likely EAGAIN)\n");
        printf("        grab      ioctl(fd, EVIOCGRAB, 1)\n");
        printf("        ungrab    ioctl(fd, EVIOCGRAB, 0)\n\n");
        printf("example:\n");
        printf("        %s 1234 flush 0 3 ungrab 3\n", argv[0]);
        printf("        will flush stdin and fd 3 for pid 1234\n");
        printf("        as well as releasing the evdev grab on fd 3\n");
        exit(1);
    }
    pid = atoi(argv[1]);
    printf("pid tracee = %d\n", pid);
    try_ptrace(PTRACE_SEIZE, pid, 0, 0);
    try_ptrace(PTRACE_GETREGS, pid, 0, &reset_regs);

    // BEGIN
    int mode = 0;

    for (int iarg = 2; iarg < argc; iarg++) {
        if (strcmp(argv[iarg], "flush") == 0) {
            mode = MODE_FLUSH;
        } else if (strcmp(argv[iarg], "grab") == 0) {
            mode = MODE_EVIOCGRAB;
        } else if (strcmp(argv[iarg], "ungrab") == 0) {
            mode = MODE_EVIOCUNGRAB;
        } else if (mode == MODE_FLUSH) {
            int fd = atoi(argv[iarg]);
            printf("flushing fd %d\n", fd);
            try(tracee_syscall(__NR_fcntl, 3, fd, F_GETFD, 0));
            int fsflags = try(tracee_syscall(__NR_fcntl, 3, fd, F_GETFL, 0));
            if (!(fsflags & O_NONBLOCK)) {
                try(tracee_syscall(__NR_fcntl, 3, fd, F_SETFL, fsflags | O_NONBLOCK));
            }
            int nread = 0;
            int ntotal = 0;
            while ((nread = tracee_syscall(__NR_read, 3, fd,
                            SP(regs) - 2048, sizeof(struct input_event))) > 0) {
                ntotal += nread;
            }
            printf(" .. flushed %d bytes\n", ntotal);
            if (!(fsflags & O_NONBLOCK)) {
                try(tracee_syscall(__NR_fcntl, 3, fd, F_SETFL, fsflags));
            }
        } else if (mode == MODE_EVIOCGRAB) {
            int fd = atoi(argv[iarg]);
            printf("grabbing fd %d\n", fd);
            printf(" .. got %ld\n", tracee_syscall(__NR_ioctl, 3, fd, EVIOCGRAB, 1));
        } else if (mode == MODE_EVIOCUNGRAB) {
            int fd = atoi(argv[iarg]);
            printf("ungrabbing fd %d\n", fd);
            printf(" .. got %ld\n", tracee_syscall(__NR_ioctl, 3, fd, EVIOCGRAB, 0));
        }
    }
    // END

    try_ptrace(PTRACE_SETREGS, pid, 0, &reset_regs);
    try_ptrace(PTRACE_DETACH, pid, 0, SIGSTOP);
    return 0;
}

#ifdef __amd64__
long tracee_syscall(long nr, long narg, ...) {
    va_list ap;
    long err;
    try_ptrace(PTRACE_GETREGS, pid, 0, &regs);
    regs.rax = nr;
    va_start(ap, narg);
        if (narg >= 1) regs.rdi = va_arg(ap, long);
        if (narg >= 2) regs.rsi = va_arg(ap, long);
        if (narg >= 3) regs.rdx = va_arg(ap, long);
        if (narg >= 4) regs.r10 = va_arg(ap, long);
        if (narg >= 5) regs.r8 = va_arg(ap, long);
        if (narg >= 6) regs.r9 = va_arg(ap, long);
    va_end(ap);
    TRACEE_ASM("syscall")
    return regs.rax;
}
#endif

#ifdef __arm__
long tracee_syscall(long nr, long narg, ...) {
    // EABI
    va_list ap;
    long err;
    try_ptrace(PTRACE_GETREGS, pid, 0, &regs);
    regs.uregs[7] = nr;
    va_start(ap, narg);
        if (narg >= 1) regs.uregs[0] = va_arg(ap, long);
        if (narg >= 2) regs.uregs[1] = va_arg(ap, long);
        if (narg >= 3) regs.uregs[2] = va_arg(ap, long);
        if (narg >= 4) regs.uregs[3] = va_arg(ap, long);
        if (narg >= 5) regs.uregs[4] = va_arg(ap, long);
        if (narg >= 6) regs.uregs[5] = va_arg(ap, long);
        if (narg >= 7) regs.uregs[6] = va_arg(ap, long);
    va_end(ap);
    TRACEE_ASMN(1, "swi 0");
    return regs.uregs[0];
}
#endif

void execute(const char * begin, const char * end, int ninstr) {
    assert((unsigned long)begin == ALIGN_DOWN((unsigned long)begin, sizeof(long)));
    assert((end - begin) > 0);
    long err;
    long buffer[512];
    const long * lbegin = (const long*)begin;
    const long * lend = (const long*)ALIGN_UP((unsigned long)end, sizeof(long));
    int nwords = lend - lbegin;

    if (IP(regs) == 0) {
        try_ptrace(PTRACE_GETREGS, pid, 0, &regs);
    }

    IP(regs) = ALIGN_DOWN(IP(regs), sizeof(long));
    const long * ip = (const long*)IP(regs);
    for (int i = 0; i < nwords; i++) {
        errno = 0;
        try_ptrace(PTRACE_PEEKTEXT, pid, &ip[i], 0);
        buffer[i] = err;
        try_ptrace(PTRACE_POKETEXT, pid, &ip[i], lbegin[i]);
    }
    try_ptrace(PTRACE_SETREGS, pid, 0, &regs);

    if (ninstr != -1) {
        for (int i = 0; i < ninstr; i++) {
            try_ptrace(PTRACE_SINGLESTEP, pid, 0, 0);
            waitpid(pid, 0, 0);
        }
    } else {
#ifdef __arm__
        puts("there is no int3 instruction on arm..");
        exit(2);
#endif
        while (1) {
            int status;
            try_ptrace(PTRACE_SINGLESTEP, pid, 0, 0);
            waitpid(pid, &status, 0);
            if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
                try_ptrace(PTRACE_GETREGS, pid, 0, &regs);
                errno = 0;
                try_ptrace(PTRACE_PEEKTEXT, pid, IP(regs), 0);
                if ((err & 0xff) == 0xcc) break;
            } else if (WIFEXITED(status)) {
                puts("tracee exit");
                exit(1);
            }
        }
    }

    for (int i = 0; i < nwords; i++) {
        try_ptrace(PTRACE_POKETEXT, pid, &ip[i], buffer[i]);
    }

    try_ptrace(PTRACE_GETREGS, pid, 0, &regs);
    try_ptrace(PTRACE_SETREGS, pid, 0, &reset_regs);
}


