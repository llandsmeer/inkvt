GITHASH='"'$(shell git rev-parse --short HEAD)'"'

CROSS_TC?=/home/llandsmeer/Build/gcc-linaro-7.5.0-2019.12-i686_arm-linux-gnueabihf/bin/arm-linux-gnueabihf
# CROSS_TC?=arm-linux-gnueabihf

ifeq ("$(DEBUG)","true")
	CFLAGS   += -g -pg
	CXXFLAGS += -g -pg
else
	CFLAGS   ?= -O2
	CXXFLAGS ?= -O2
endif

CPPFLAGS += -Ilibvterm/include -DGITHASH=$(GITHASH)
CFLAGS   += -Wall -falign-labels=8
CXXFLAGS += -Wall -falign-labels=8

# All the warnings! \o/
EXTRA_WARNINGS+=-Wextra -Wunused
EXTRA_WARNINGS+=-Wformat=2
EXTRA_WARNINGS+=-Wformat-signedness
# NOTE: -Wformat-truncation=2 is still a tad too aggressive w/ GCC 9, so, tone it down to avoid false-positives...
EXTRA_WARNINGS+=-Wformat-truncation=1
EXTRA_WARNINGS+=-Wnull-dereference
EXTRA_WARNINGS+=-Wuninitialized
ifeq (flto,$(findstring flto,$(CFLAGS)))
	# Much like SQLite, libvterm also trips quite a few -Wnull-dereference warnings at link-time w/ LTO
	EXTRA_WARNINGS+=-Wno-null-dereference
	# And also a few -Wmaybe-uninitialized ones
	EXTRA_WARNINGS+=-Wno-maybe-uninitialized
endif
EXTRA_WARNINGS+=-Wduplicated-branches -Wduplicated-cond
EXTRA_WARNINGS+=-Wundef
EXTRA_WARNINGS+=-Wwrite-strings
EXTRA_WARNINGS+=-Wlogical-op
EXTRA_WARNINGS+=-Wshadow
#EXTRA_WARNINGS+=-Wmissing-declarations
#EXTRA_WARNINGS+=-Winline
EXTRA_WARNINGS+=-Wcast-qual
# NOTE: GCC 8 introduces -Wcast-align=strict to warn regardless of the target architecture (i.e., like clang)
EXTRA_WARNINGS+=-Wcast-align
EXTRA_WARNINGS+=-Wconversion
# Output padding info when debugging (NOTE: Clang is slightly more verbose)
# As well as function attribute hints
ifdef DEBUG
	EXTRA_WARNINGS+=-Wpadded
	EXTRA_WARNINGS+=-Wsuggest-attribute=pure -Wsuggest-attribute=const -Wsuggest-attribute=noreturn -Wsuggest-attribute=format -Wmissing-format-attribute
endif

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
	python3 src/kblayout.py > src/_kblayout.hpp
	g++ $(CPPFLAGS) $(CXXFLAGS) $(EXTRA_WARNINGS) src/main.cpp -lvterm -lfbink -o build/inkvt.host $(LDFLAGS)
ifneq ("$(DEBUG)","true")
	strip --strip-unneeded build/inkvt.host
endif

kobo: build/fbdepth build/libfbink_kobo.a build/libvterm_kobo.a src/_kbsend.hpp
	python3 keymap.py > src/_keymap.hpp
	python3 src/kblayout.py > src/_kblayout.hpp
	$(CROSS_TC)-g++ -DTARGET_KOBO $(CPPFLAGS) $(CXXFLAGS) $(EXTRA_WARNINGS) src/main.cpp -lvterm_kobo -lfbink_kobo -o build/inkvt.armhf $(LDFLAGS) $(STATIC_STL_FLAG)
	$(CROSS_TC)-strip --strip-unneeded build/inkvt.armhf
	upx build/inkvt.armhf || echo "install UPX for smaller executables"

release: clean kobo
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
	make -C FBInk clean || (echo "TRY git submodule update --init --recursive"  && false)
	env -u CROSS_TC -u CPPFLAGS -u CFLAGS -u CXXFLAGS -u LDFLAGS -u AR -u RANLIB make -C FBInk LINUX=true MINIMAL=true FONTS=true IMAGE=true staticlib
	cp FBInk/Release/libfbink.a build/libfbink.a

build/libfbink_kobo.a:
	mkdir -p build
	make -C FBInk clean
	make -C FBInk CROSS_TC=$(CROSS_TC) KOBO=true MINIMAL=true FONTS=true IMAGE=true staticlib
	cp FBInk/Release/libfbink.a build/libfbink_kobo.a

build/fbdepth:
	mkdir -p build
	make -C FBInk clean
	make -C FBInk CROSS_TC=$(CROSS_TC) KOBO=true utils
	cp FBInk/Release/fbdepth build/fbdepth

clean:
	make -C FBInk clean  || (echo "TRY git submodule update --init --recursive"  && false)
	make -f Makevterm clean
	rm -fr build/
	rm -f InkVT-*.zip
