GITHASH='"'$(shell git rev-parse --short HEAD)'"'

# CROSS_TC?=/home/llandsmeer/Build/gcc-linaro-7.5.0-2019.12-i686_arm-linux-gnueabihf/bin/arm-linux-gnueabihf
CROSS_TC?=arm-linux-gnueabihf

ifeq ("$(DEBUG)","true")
	CFLAGS   += -g -pg
	CXXFLAGS += -g -pg
else
	CFLAGS   ?= -O2
	CXXFLAGS ?= -O2
endif

CPPFLAGS += -Ilibvterm-0.1.3/include -DGITHASH=$(GITHASH)
CFLAGS   += -Wall -falign-labels=8
CXXFLAGS += -Wall -falign-labels=8 -fpermissive

# Attempt to automatically drop -static-libstdc++ when using the Nickel TC...
ifeq ($(shell PATH='$(PATH)' $(CROSS_TC)-gcc -dumpmachine 2>/dev/null), arm-nickel-linux-gnueabihf)
	STATIC_STL_FLAG:=
	# What can I say, it's old...
	CXXFLAGS += -std=gnu++14
else
	STATIC_STL_FLAG:= -static-libstdc++
	CXXFLAGS += -std=gnu++17
endif

ifdef INPUT_SERIAL
	CPPFLAGS += -DINPUT_SERIAL
endif

LDFLAGS+=-lm -Lbuild -lutil

all: linux kobo
.PHONY: linux kobo clean all

src/_kbsend.hpp: src/kbsend.html
	xxd -i src/kbsend.html > src/_kbsend.hpp || echo "install xxd to update src/_kbsend.hpp"

linux: build/libfbink.a build/libvterm.a src/_kbsend.hpp
	python3 keymap.py > src/_keymap.hpp
	g++ $(CPPFLAGS) $(CXXFLAGS) src/main.cpp -lvterm -lfbink -o build/inkvt.host $(LDFLAGS)
ifneq ("$(DEBUG)","true")
	strip --strip-unneeded build/inkvt.host
endif

kobo: build/fbdepth build/libfbink_kobo.a build/libvterm_kobo.a src/_kbsend.hpp
	python3 keymap.py > src/_keymap.hpp
	$(CROSS_TC)-g++ -DTARGET_KOBO $(CPPFLAGS) $(CXXFLAGS) src/main.cpp -lvterm_kobo -lfbink_kobo -o build/inkvt.armhf $(LDFLAGS) $(STATIC_STL_FLAG)
	$(CROSS_TC)-strip --strip-unneeded build/inkvt.armhf
	upx build/inkvt.armhf || echo "install UPX for smaller executables"

release: kobo
	mkdir -p Kobo/.adds/inkvt Kobo/.adds/kfmon/config
	cp -av $(CURDIR)/build/inkvt.armhf Kobo/.adds/inkvt
	cp -av $(CURDIR)/build/fbdepth Kobo/.adds/inkvt
	cp -av $(CURDIR)/koboroot/.adds/inkvt/. Kobo/.adds/inkvt/.
	cp -av $(CURDIR)/koboroot/inkvt.png Kobo/
	cp -av $(CURDIR)/koboroot/inkvt.ini Kobo/.adds/kfmon/config/inkvt.ini
	cd Kobo && zip -r ../InkVT-$(shell git rev-parse --short HEAD).zip .
	rm -rf Kobo

build/libvterm.a:
	make -f Makevterm clean
	make -f Makevterm NATIVE_TC=1

build/libvterm_kobo.a:
	make -f Makevterm clean
	make CROSS_TC=$(CROSS_TC) OUT=libvterm_kobo.a -f Makevterm

build/libfbink.a:
	mkdir -p build
	make -C FBInk clean
	env -u CROSS_TC -u CPPFLAGS -u CFLAGS -u CXXFLAGS -u LDFLAGS -u AR -u RANLIB make -C FBInk LINUX=true MINIMAL=true FONTS=true staticlib
	cp FBInk/Release/libfbink.a build/libfbink.a

build/libfbink_kobo.a:
	mkdir -p build
	make -C FBInk clean
	make -C FBInk CROSS_TC=$(CROSS_TC) KOBO=true MINIMAL=true FONTS=true staticlib
	cp FBInk/Release/libfbink.a build/libfbink_kobo.a

build/fbdepth:
	mkdir -p build
	make -C FBInk clean
	make -C FBInk CROSS_TC=$(CROSS_TC) KOBO=true utils
	cp FBInk/Release/fbdepth build/fbdepth

clean:
	make -C FBInk clean
	make -f Makevterm clean
	rm -fr build/
	rm -f InkVT-*.zip
