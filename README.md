*Warning!! I have stopped actively developing this project, sold off my kobo and switched to the pinenote. I'll happily merge Pull Requests and provide feedback, but I don't have access to a kobo device currently to test bugs and new code. If someone wants to lead development in this project I can add a link to your fork here*

Experimental VT100 terminal emulator for the Kobo Libra H2O (and probably all targets supported by FBInk)

<img src=it_works.jpeg width=800/>
<img src=it_works2.png width=800/>

# Install on Kobo

Warning: this project is very experimental!
It might brick your device.
Only install this if you know what you are doing.
If anything breaks, let me know!
I'd like turn this into a stable piece of software eventually :)
The kfmon/nickel interaction code is stolen from [KOReader](https://github.com/koreader/koreader)

The installation assumes you have [KFMon](https://github.com/NiLuJe/kfmon) installed

**IMPORTANT:** The `make kobo` target expects a working `arm-eabihf` cross-compiler.
The Makefile by default assumes the cross-compilers in the apt standard repository to be present (which probably lead to strange glibc version mismatches and crashes).
This comprises the ubuntu `gcc-arm-linux-gnueabihf`, `libc6-dev-armhf-cross`, `g++-arm-linux-gnueabihf`  and `libstdc++-4.8-dev-armhf-cross`
packages.
The [KOReader toolchains](https://github.com/koreader/koxtoolchain) are also supported,
and **will in fact be a much better fit for the target device than Ubuntu/Linaro/MG/Sourcery ones**.
This has been tested with the "kobo" one.
You can also use the "nickel" one if you really want a crappier GCC version,
whose only saving grace will be the ability to link against the STL dynamically.
The Makefile will honor the `CROSS_TC` variable, which is setup by the env script as documented in koxtoolchain.
Pass DEBUG=1 (eg. `make clean; make DEBUG=1 kobo`) to create a debug build.

```
$ sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf
$ git clone 'https://github.com/llandsmeer/inkvt'
$ cd inkvt
$ git submodule update --init --recursive
$ make clean && make release
```

Then, in a standard USBMS session, unpack the `InkVT-*.zip` archive to the root of your Kobo device.
Eject & unplug it safely, and let it process the new InkVT "book".

NOTE:
When inkvt starts, it will start wifi (so before you start you must have a stable wifi connection)
and wait 10 seconds.
After that the actual inkvt binary is run.
If your internet connection failed, or anything else that won't
enable you to input text, it will shut down again after 20 seconds.
This is the amount of time you have to move to the local network
`http://ip:7800/` address shown on the screen.
If you type here, your keystrokes are transfered to the device.
Over *plain* http. Thats not secure!

If you need more tools for your shell, check out [NiLuJe's repository](https://github.com/llandsmeer/inkvt/pull/2#issuecomment-605522605).

# Inkvt

So recently I bought a Kobo Libra H2O and decided that it needs a terminal
emulator. The main problem is now input.
The kernel that comes with the device doesn't support usb host mode,
but it (OTG) is supported by the hardware. I use my laptop as host,
which sends keystrokes over serial to the Kobo device via the `g_serial`
loadable kernel module that *may* come with the device.
This option is enabled by  the `--serial` command line flag (edit inkvtsh, around line 140).
The other option is sending keystrokes over wifi, which doesn't require
loading kernel modules and is more cross-platform (the default).

`inkvt` is mostly a wrapper around two really nice libraries:

  - [FBInk](https://github.com/NiLuJe/FBInk/)
  - [libvterm](http://www.leonerd.org.uk/code/libvterm/)

And since `libvterm` comes without documentation, I am thankful for the
[x86term](https://github.com/pkovac/x86term) project for nicely showing
how to use the api.

# Local build & install

`make` generates 2 binaries: `build/inkvt.armhf` and `build/inkvt.host`.
The first one targets the kobo, the second one the host's linux (for development & testing).
If you want to try this on a desktop linux, run it outside
X/Wayland using <kbd>Ctrl+Alt+F3</kbd>.

To send keyboard input, there are 3 options:
 - If you started inkvt from ssh, stdin
 - By sending keyboard input with a http post request to port 7800.
   If you move your browser to the ip adress where inkvt is listening (`127.0.0.1:7800` for
   a local test), you'll end up with a webpage where you can send text to inkvt.
   The idea is that you can use a phone with an OTG keyboard.
   It requires that you're on the same (wifi) network, so for example, the builtin
   phone hotspot or a common wifi router. Also, android keyinput is extremely buggy...
   To enable this, remove `--no-http` from `inkvt.sh`.
 - (via `--serial`): `screen /dev/ttyACM0 9600` (this requires a Mk. 7+ device, or a self-built g_serial module):
   Edit `koboroot/.adds/inkvt/inkvt.sh` around line 140 to change startup options.
 - On screen keyboard (very experimental, via `--osk`). The last output row is not shown and Ctrl/Alt
   does not work yet. Rotations are not tested either.

# Authors/Contributors

 - [NiLuJe](https://github.com/llandsmeer/inkvt/commits?author=NiLuJe)
 - [llandsmeer](https://github.com/llandsmeer/inkvt/commits?author=llandsmeer)

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

<!-- kate: indent-mode cstyle; indent-width 4; replace-tabs on; remove-trailing-spaces none; -->
