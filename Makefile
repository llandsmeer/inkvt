GITHASH='"'$(shell git log --format="%H" -n 1)'"'

CROSS_TC?=/home/llandsmeer/Build/gcc-linaro-7.5.0-2019.12-i686_arm-linux-gnueabihf/bin/arm-linux-gnueabihf

ifeq ("$(DEBUG)","true")
	CFLAGS   += -g -pg
	CXXFLAGS += -g -pg
else
	CFLAGS   ?= -O2
	CXXFLAGS ?= -O2
endif

CPPFLAGS += -Ilibvterm-0.1.3/include -DGITHASH=$(GITHASH)
CFLAGS   += -Wall -falign-labels=8
CXXFLAGS += -Wall -falign-labels=8 -std=gnu++17

ifdef INPUT_EVDEV
	CPPFLAGS += -DINPUT_EVDEV
endif

ifdef INPUT_SERIAL
	CPPFLAGS += -DINPUT_SERIAL
endif

LDFLAGS+=-lm -Lbuild -lutil

all: tracexec linux kobo
.PHONY: tracexec linux kobo clean all

build/tsyscall.x:
	mkdir -p build
	gcc src/tsyscall.cpp -Wall -masm=intel -falign-labels=8 -Wno-unused-value -o build/tsyscall.x86
	$(CROSS_TC)-gcc src/tsyscall.cpp -Wall -falign-labels=8 -Wno-unused-value -o build/tsyscall.x

src/_kbsend.hpp: src/kbsend.html
	xxd -i src/kbsend.html > src/_kbsend.hpp || echo "install xxd to update src/_kbsend.hpp"

linux: build/libfbink.a build/libvterm.a src/_kbsend.hpp
	python3 keymap.py > src/_keymap.hpp
	g++ $(CPPFLAGS) $(CXXFLAGS) src/main.cpp -lvterm -lfbink -o build/inkvt.host $(LDFLAGS)
ifneq ("$(DEBUG)","true")
	strip --strip-unneeded build/inkvt.host
endif
ifeq ($(INPUT_EVDEV),"true")
	g++ $(CPPFLAGS) $(CXXFLAGS) src/evdev2serial.cpp -o build/evdev2serial.x86 $(LDFLAGS)
endif

kobo: build/libfbink_kobo.a build/libvterm_kobo.a src/_kbsend.hpp
	python3 keymap.py > src/_keymap.hpp
	$(CROSS_TC)-g++ -static -DTARGET_KOBO $(CPPFLAGS) $(CXXFLAGS) src/main.cpp -lvterm_kobo -lfbink_kobo -o build/inkvt.armhf $(LDFLAGS)
	$(CROSS_TC)-strip --strip-unneeded build/inkvt.armhf
	upx build/inkvt.armhf || echo "install UPX for smaller executables"

build/libvterm.a:
	make -f Makevterm clean
	make -f Makevterm NATIVE_TC=1

build/libvterm_kobo.a:
	make -f Makevterm clean
	make CROSS_TC=$(CROSS_TC) OUT=libvterm_kobo.a -f Makevterm

build/libfbink.a:
	mkdir -p build
	make -C FBInk clean
	env -u CROSS_TC -u CPPFLAGS -u CFLAGS -u CXXFLAGS -u LDFLAGS -u AR -u RANLIB make -C FBInk LINUX=true MINIMAL=true FONTS=true static
	cp FBInk/Release/libfbink.a build/libfbink.a

build/libfbink_kobo.a:
	mkdir -p build
	make -C FBInk clean
	make -C FBInk CROSS_TC=$(CROSS_TC) KOBO=true MINIMAL=true FONTS=true static
	cp FBInk/Release/libfbink.a build/libfbink_kobo.a

clean:
	make -C FBInk clean
	make -f Makevterm clean
	rm -fr build/
