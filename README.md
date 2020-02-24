# Inkvt

WIP VT100 terminal emulator for the Kobo Libra H2O (and probably all targets supported by FBInk)

<img src=it_works.jpeg width=400 align=right />

So recently I bought a Kobo Libra H2O and decided that it needs a terminal
emulator. The main problem is now input.
The kernel that comes with the device doesn't support usb host mode,
but it (OTG) is supported by the hardware. For now I use my laptop as host,
which sends keystrokes over serial to the Kobo device via the `g_serial`
loadable kernel module that comes with the device.


`inkvt` is mostly a wrapper around two really nice libraries:

  - [FBInk](https://github.com/NiLuJe/FBInk/)
  - [libvterm](http://www.leonerd.org.uk/code/libvterm/)

And since `libvterm` comes without documentation, I am thankful for the
[x86term](https://github.com/pkovac/x86term) project for nicely showing
how to use the api.

# Install & Usage

The `make kobo` target expects a working `arm-eabihf` cross-compiler.
I downloaded mine from [Linaro](https://releases.linaro.org/components/toolchain/binaries/latest-7/arm-linux-gnueabihf/)
(as that seems to be the one the Kobo team uses), but other builds might work just as fine.
Then update the `CROSS_TC` variable in the Makefile.

```
$ git clone 'https://github.com/llandsmeer/inkvt'
$ cd inkvt
$ make
[...]
```

Which should generate 3 binaries:

 - `build/vterm.xarm`: inkvt built for kobo.
    Sets up serial over USB and listens to `/dev/input/event*` and `/dev/ttyGS0` for evdev
    input.
 - `build/evdev2serial.x86`: listens to evdev devices and send `EV_KEY` events to `/dev/ttyACM0`.
 - `build/vterm.x86`: inkvt built for linux framebuffer (for development). Listens to `/dev/input/event*`.
    Only usable from linux console (eg. <kbd>Ctrl+Super+F3</kbd>).

So to run, copy `./build/vterm.xarm` to `/mnt/onboard`, SSH into the Kobo device and run `./vterm.xarm`.
Then, connect USB and run `sudo ./build/evdev2serial.x86` from linux.

# Todo

 - Implement `vterm` callback `moverect`
 - Implement `vterm` callback `movecursor`
 - Switch default `sh` to `bash`
 - Make it runnable from KFMon and KOReader. I'm thinking, reading `/proc/*/fd` for processes
   that are reading `/dev/input/event*`, `SIGSTOP`-ing them and using `tracexec` to
   ungrab and drain their evdev devices. Or maybe even temporary close the file.
   This would also mean that I have to catch all signals/segv and handle them gracefully.
   Or just copy the setup scripts from KOReader.
 - Add hideable touchscreen keyboard / settings bar
 - Lock: detect if inkterm is already running and fail if so.
   Executing `evdev2serial` and then running `./vterm.xarm` usually results in inkvt
   starting twice, because the keypresses that start it are also send to inkvt over serial...
 - Get Ctrl-<KEY> escape sequences working for keys other than standard printable ascii.
   [This](https://invisible-island.net/xterm/ctlseqs/ctlseqs.html) might be of help.
 - Connect an actual hardware keyboard. Simplest solution would be putting a Raspberry Pi between
   the keyboard and kobo and running `build/evdev2serial.x86`, compiled on the Raspberry I think.
 - Detect `pty(7)` slave `sh` exit.
 - Disable stdin printing while `build/vterm.x86` runs. Probably just `noecho`.
   Currently linux console and inkvt fight over the framebuffer.
   Maybe limit stdout/stderr too/move printfs to something that that be configured
   to output to the framebuffer.
 - Handle screen rotation
 - Implement `vterm` callback `settermprop`
 - Switch to vectorized font. Preferable something typewriter like. And include the
   file as a raw file in the output binary s.t. everything stays in 1 file.

# Tracexec

After [some discussion](https://github.com/NiLuJe/FBInk/issues/45), I decided to
try to build a program which can break into Nickel and drain evdev events.
So `tracexec.c` was born. Its quite flexible and can be used to execute arbitrary
assembly/syscalls inside other processes on `amd64` and `arm-eabi`:

```c
int main(int argc, char ** argv) {
    long err;
    if (argc < 2) { printf("usage:  %s PID\n", argv[0]); exit(1); }
    pid = atoi(argv[1]);
    try_ptrace(PTRACE_SEIZE, pid, 0, 0);
    try_ptrace(PTRACE_GETREGS, pid, 0, &reset_regs);

    // execute a syscall inside tracee
    int syscall_pid = tracee_syscall(__NR_getpid, 0);
    printf("tracee pid: %d\n", syscall_pid);

    // execute assembly inside tracee
    TRACEE_ASM(
        "push 0x2048454c\n"
        "mov rax, 1\n"
        "mov rdi, 1\n"
        "mov rsi, rsp\n"
        "mov rdx, 4\n"
        "syscall\n"
        "pop rax\n"
    )

    try_ptrace(PTRACE_SETREGS, pid, 0, &reset_regs);
    try_ptrace(PTRACE_DETACH, pid, 0, SIGSTOP);
    return 0;
}
```

# Related projects:

 - [fbpad-eink](https://github.com/kisonecat/fbpad-eink)

# License

```
inkvt - VT100 terminal for E-ink devices
Copyright (C) 2020 Lennart Landsmeer <lennart@landsmeer.email>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
```
