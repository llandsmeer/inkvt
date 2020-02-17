CROSS_TC=/home/llandsmeer/Build/gcc-linaro-7.5.0-2019.12-i686_arm-linux-gnueabihf/bin/arm-linux-gnueabihf
CPPFLAGS=-Wall
LDFLAGS=-lm -Lbuild -lutil

all: tracee_exec linux kobo
.PHONY: tracee_exec linux kobo clean all

tracee_exec:
	mkdir -p build
	gcc src/tracexec.c -Wall -masm=intel -falign-labels=8 -Wno-unused-value -o build/tracexec.x86
	$(CROSS_TC)-gcc src/tracexec.c -Wall -falign-labels=8 -Wno-unused-value -o build/tracexec.x

linux: build/libfbink.a
	g++ src/main.cpp -lfbink -o build/vterm.x86 $(LDFLAGS) $(CPPFLAGS)

kobo: build/libfbink_kobo.a
	$(CROSS_TC)-g++ -DTARGET_KOBO src/main.cpp -lfbink_kobo -o build/vterm.xarm $(LDFLAGS) $(CPPFLAGS)

build/libfbink.a:
	mkdir -p build
	make -C FBInk clean
	make -C FBInk LINUX=true static
	cp FBInk/Release/libfbink.a build/libfbink.a

build/libfbink_kobo.a:
	mkdir -p build
	make -C FBInk clean
	make -C FBInk CROSS_TC=$(CROSS_TC) KOBO=true static
	cp FBInk/Release/libfbink.a build/libfbink_kobo.a

clean:
	make -C FBInk clean
	rm -fr build/
