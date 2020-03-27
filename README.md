Experimental VT100 terminal emulator for the Kobo Libra H2O (and probably all targets supported by FBInk)

<img src=it_works.jpeg width=800/>

# Install on Kobo

Warning: this project is very experimental!
It might brick your device.
Only install this if you know what you are doing.
If anything breaks, let me know!
I'd like turn this into a stable piece of software eventually :)
The kfmon/nickel interaction code is stolen from [KOReader](https://github.com/koreader/koreader)

The installation assumes you have [kfmon](https://github.com/NiLuJe/kfmon) installed

The `make kobo` target expects a working `arm-eabihf` cross-compiler.
I downloaded mine from [Linaro](https://releases.linaro.org/components/toolchain/binaries/latest-7/arm-linux-gnueabihf/)
(as that seems to be the one the Kobo team uses).
The ubuntu `gcc-arm-linux-gnueabihf`, `libc6-dev-armhf-cross`, `g++-arm-linux-gnueabihf`  and `libstdc++-4.8-dev-armhf-cross`
packages might work too.
Then update the `CROSS_TC` variable in the Makefile.

```
$ git clone 'https://github.com/llandsmeer/inkvt'
$ cd inkvt
make
cp build/inkvt.armhf koboroot/.adds/inkvt/
```

Then copy the contents of the `koboroot/` directory to your kobo device:
 - `koboroot/.adds/inkvt` to `/.adds/inkvt`
 - `koboroot/inkvt.png` to `/inkvt.png`
 - `koboroot/inkvt.ini` to `/.adds/kfmon/config/inkvt.ini`

Then restart your device and *read this:*
When inkvt starts, it will start wifi (so before you start you must have a stable wifi connection)
and wait 10 seconds.
After that the actual inkvt binary is run.
If your internet connection failed, or anything else that won't
enable you to input text, it will shut down again after 20 seconds.
This is the amount of time you have to move to the local network
`http://ip:7800/` address shown on the screen.
If you type here, your keystrokes are transfered to the device.
Over *plain* http. Thats not secure!

# Inkvt

So recently I bought a Kobo Libra H2O and decided that it needs a terminal
emulator. The main problem is now input.
The kernel that comes with the device doesn't support usb host mode,
but it (OTG) is supported by the hardware. I use my laptop as host,
which sends keystrokes over serial to the Kobo device via the `g_serial`
loadable kernel module that *may* come with the device.
This option is enabled by passing `INPUT_SERIAL=true` to make.
The other option is sending keystrokes over wifi, which doesn't require
loading kernel modules and is more cross-platform (the default).

`inkvt` is mostly a wrapper around two really nice libraries:

  - [FBInk](https://github.com/NiLuJe/FBInk/)
  - [libvterm](http://www.leonerd.org.uk/code/libvterm/)

And since `libvterm` comes without documentation, I am thankful for the
[x86term](https://github.com/pkovac/x86term) project for nicely showing
how to use the api.

# Local build & install

```
$ make
[...]
```

Which generates 2 binaries: `build/inkvt.armhf` and `build/inkvt.host`.
The first one targets the kobo, the second one the host's linux.
If you want to try this on a desktop linux, run it outside
X/Wayland using <kbd>Ctrl+Alt+F3</kbd>.

To send keyboard input, there are 3 options:
 - if you started inkvt from ssh, stdin
 - By sending keyboard input with a http post request to port 7800.
   If you move your browser to the ip adress where inkvt is listening (`127.0.0.1:7800` for
   a local test), you'll end up with a webpage where you can send text to inkvt.
   The idea is that you can use a phone with an OTG keyboard.
   It requires that you're on the same (wifi) network, so for example, the builtin
   phone hotspot or a common wifi router.
   Also, android keyinput is extremely buggy...
 - (disabled by default) `screen /dev/ttyACM0 9600` (this requires a Mk. 7+ device, or a self-built g_serial module):

# Alternative input method (evdev & evdev over serial)

Before tty raw input, I build a keyboard input system around evdev.
That's a lot less portable than tty raw input, and will probably be obsoleted somewhere in
the future.
Nevertheless, if you need this, passing `INPUT_EVDEV=true` to make should generate 3 binaries:

 - `build/inkvt.armhf`: inkvt built for kobo.
    Sets up serial over USB and listens to `/dev/input/event*` and `/dev/ttyGS0` for evdev
    input.
 - `build/evdev2serial.x86`: listens to evdev devices and send `EV_KEY` events to `/dev/ttyACM0`.
 - `build/inkvt.host`: inkvt built for linux framebuffer (for development). Listens to `/dev/input/event*`.
    Only usable from linux console (eg. <kbd>Ctrl+Super+F3</kbd>).

So to run, copy `./build/inkvt.armhf` to `/mnt/onboard`, SSH into the Kobo device and run `./inkvt.armhf`.
Then, connect USB and run `sudo ./build/evdev2serial.x86` from linux.

# OTA

For development, I found the follow workflow eases deployment:
Run `python3 -m http.server` from the `inkvt/build` directory.
Then after each `make`, just `wget` on the Kobo side.

# Todo

 - Add hideable touchscreen keyboard / settings bar
 - Far future: switch to vectorized font. Only for ligatures :).
   Terminus is fine for now.
   Preferable something typewriter like. And include the
   file as a raw file in the output binary s.t. everything stays in 1 file.

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
