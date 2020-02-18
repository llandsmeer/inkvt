# Inkvt

WIP VT100 terminal emulator for the Kobo Libra H2O (and probably all targets supported by FBInk)

<img src=it_works.jpeg width=400 align=right />

So recently I bought a Kobo Libra H2O and decided that it needs a terminal
emulator. The main problem is now input.
The kernel that comes with the device doesn't support usb host mode,
but it (OTG) is supported by the hardware. For now I use my laptop as host,
which sends keystrokes over serial to the Kobo device via the `g_serial`
loadable kernel module that comes with the device.

`screen /dev/ttyS0 -s 9600`

`inkvt` is mostly a wrapper around two really nice libraries:

  - [FBInk](https://github.com/NiLuJe/FBInk/)
  - [libvterm](http://www.leonerd.org.uk/code/libvterm/)

And since `libvterm` comes without documentation, I am thankful for the
[x86term](https://github.com/pkovac/x86term) project for nicely showing
how to use the api.

Todo:

 - Reliably get escape sequences working. Maybe pick an escape character
   to send control codes?
 - Connect an actual hardware keyboard
 - Make it runnable from KFMon
# Tracexec

After [some discussion](https://github.com/NiLuJe/FBInk/issues/45), I decided to
try to build a program which can break into Nickel and drain evdev events.
So `tracexec.c` was born. Its quite flexible and can be used to execute arbitrary
assembly/syscalls inside other processes on `amd64` and `arm-eabi`.

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
