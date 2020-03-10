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

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>

static long ttry_(long err, const char * cmd, int line) {
    if (err < 0 && errno != 0) {
        printf("%s:%d: try failure\n", __FILE__, line);
        perror(cmd);
        exit(1);
    }
    return err;
}

#define ttry(x) ttry_(x, #x, __LINE__)

#ifdef __arm__
static long tsyscall(pid_t pid, long number, ...) {
    struct user_regs regs_orig, regs;
    long asm_orig[2];
    static unsigned long asm_syscall[2] = {
        /* swi 0    */ 0xef000000,
        /* udf 0x10 */ 0xe7f001f0,
    };
    va_list ap;
    ttry(ptrace(PTRACE_GETREGS, pid, 0, &regs_orig));
    regs = regs_orig;
    va_start(ap, number);
    regs.uregs[7] = number;
    regs.uregs[0] = va_arg(ap, long);
    regs.uregs[1] = va_arg(ap, long);
    regs.uregs[2] = va_arg(ap, long);
    regs.uregs[3] = va_arg(ap, long);
    regs.uregs[4] = va_arg(ap, long);
    regs.uregs[5] = va_arg(ap, long);
    regs.uregs[6] = va_arg(ap, long);
    regs.uregs[15] = regs_orig.uregs[15] & ~(sizeof(long)-1);
    va_end(ap);
    if (regs.uregs[16] & 0x20) {
        /* cpsr thumb mode bit set. */
        regs.uregs[16] = regs.uregs[16] & ~0x20;
    }
    long * ip = (long*)regs.uregs[15];
    errno = 0; /* must do this for error checking in peektext */
    asm_orig[0] = ttry(ptrace(PTRACE_PEEKTEXT, pid, ip, 0));
    asm_orig[1] = ttry(ptrace(PTRACE_PEEKTEXT, pid, ip+1, 0));
    ttry(ptrace(PTRACE_SETREGS, pid, 0, &regs));
    ttry(ptrace(PTRACE_POKETEXT, pid, ip, asm_syscall[0]));
    ttry(ptrace(PTRACE_POKETEXT, pid, ip+1, asm_syscall[1]));
    int wstatus;
    while ((long)regs.uregs[15] == (long)ip) {
        ttry(ptrace(PTRACE_CONT, pid, 0, 0));
        ttry(waitpid(pid, &wstatus, 0));
        if (WIFEXITED(wstatus)) { return WEXITSTATUS(wstatus); }
        ttry(ptrace(PTRACE_GETREGS, pid, 0, &regs));
    }
    ttry(ptrace(PTRACE_POKETEXT, pid, ip, asm_orig[0]));
    ttry(ptrace(PTRACE_POKETEXT, pid, ip+1, asm_orig[1]));
    ttry(ptrace(PTRACE_SETREGS, pid, 0, &regs_orig));
    return regs.uregs[0];
}
static void known_state(pid_t pid) {
    // now, there is the possibility were in the middle
    // of a syscall we'll just let that one execute
    // thus possible hang here, during a long read()
    struct user_regs regs;
    int wstatus;
    ttry(waitpid(pid, &wstatus, WNOHANG));
    if (!WIFSTOPPED(wstatus)) {
        kill(pid, SIGSTOP);
        ttry(waitpid(pid, &wstatus, 0));
        assert(WIFSTOPPED(wstatus) && WSTOPSIG(wstatus) == SIGSTOP);
        ttry(ptrace(PTRACE_GETREGS, pid, 0, &regs));
        errno = 0;
        long orig = ttry(ptrace(PTRACE_PEEKTEXT, pid, regs.uregs[15], 0));
        ttry(ptrace(PTRACE_POKETEXT, pid, regs.uregs[15], 0xe7f001f0));
        ttry(ptrace(PTRACE_CONT, pid, 0, SIGCONT));
        ttry(waitpid(pid, &wstatus, 0));
        assert(WIFSTOPPED(wstatus) && WSTOPSIG(wstatus) == SIGTRAP);
        ttry(ptrace(PTRACE_POKETEXT, pid, regs.uregs[15], orig));
        ttry(ptrace(PTRACE_GETREGS, pid, 0, &regs));
    } else {
        tsyscall(pid, __NR_getpid);
    }
}
#endif

#ifdef __amd64__
static long tsyscall(pid_t pid, long number, ...) {
    struct user_regs_struct regs_orig, regs;
    long asm_orig;
    va_list ap;
    ttry(ptrace(PTRACE_GETREGS, pid, 0, &regs_orig));
    regs = regs_orig;
    va_start(ap, number);
    static uint8_t asm_syscall[8] __attribute__ ((aligned (8))) = {
        /* syscall*/ 0x0f, 0x05,
        /* ud2    */ 0x0f, 0x0b,
        /* zeros  */ 0, 0, 0, 0};
    regs.rax = number;
    regs.rdi = va_arg(ap, long);
    regs.rsi = va_arg(ap, long);
    regs.rdx = va_arg(ap, long);
    regs.r10 = va_arg(ap, long);
    regs.r8 = va_arg(ap, long);
    regs.r9 = va_arg(ap, long);
    regs.rip = regs_orig.rip & ~(sizeof(long)-1);
    void * ip = (void*)regs.rip;
    assert(sizeof(asm_orig) == sizeof(asm_syscall));
    va_end(ap);
    errno = 0; /* must do this for error checking in peektext */
    asm_orig = ttry(ptrace(PTRACE_PEEKTEXT, pid, ip, 0));
    ttry(ptrace(PTRACE_SETREGS, pid, 0, &regs));
    ttry(ptrace(PTRACE_POKETEXT, pid, ip, *(long*)asm_syscall));
    ttry(ptrace(PTRACE_SINGLESTEP, pid, 0, 0));
    int wstatus;
    ttry(waitpid(pid, &wstatus, 0));
    if (WIFEXITED(wstatus)) { return WEXITSTATUS(wstatus); }
    ttry(ptrace(PTRACE_GETREGS, pid, 0, &regs));
    ttry(ptrace(PTRACE_POKETEXT, pid, ip, asm_orig));
    ttry(ptrace(PTRACE_SETREGS, pid, 0, &regs_orig));
    return regs.rax;
}
static void known_state(pid_t pid) {
    int wstatus;
    kill(pid, SIGSTOP);
    ttry(waitpid(pid, &wstatus, 0));
    assert(WIFSTOPPED(wstatus) && WSTOPSIG(wstatus) == SIGSTOP);
}
#endif

#ifdef __cplusplus
struct tsyscall {
#ifdef __amd64__
    struct user_regs_struct regs;
    long sp() { return regs.rsp; }
#elif __arm__
    struct user_regs regs;
    long sp() { return regs.uregs[13]; }
#endif
    pid_t pid;
    void begin(pid_t pid) {
        this->pid = pid;
        ttry(ptrace(PTRACE_SEIZE, pid, 0, 0));
        known_state(pid);
        ttry(ptrace(PTRACE_GETREGS, pid, 0, &regs));
    }
    void end() {
        ttry(ptrace(PTRACE_DETACH, pid, 0, SIGCONT));
    }
    long syscall(long number, ...) {
        va_list ap;
        va_start(ap, number);
        long arg1 = va_arg(ap, long);
        long arg2 = va_arg(ap, long);
        long arg3 = va_arg(ap, long);
        long arg4 = va_arg(ap, long);
        long arg5 = va_arg(ap, long);
        long arg6 = va_arg(ap, long);
        long res = ::tsyscall(pid, number, arg1, arg2, arg3, arg4, arg5, arg6);
        va_end(ap);
        return res;
    }

    long ioctl(int fd, unsigned int cmd, unsigned long arg) {
        return syscall(__NR_ioctl, fd, cmd, arg);
    }

    long fcntl(int fd, unsigned int cmd, unsigned long arg) {
        return syscall(__NR_fcntl, fd, cmd, arg);
    }

    long read(int fd, void * buf, size_t count) {
        return syscall(__NR_read, fd, buf, count);
    }
};
#endif
