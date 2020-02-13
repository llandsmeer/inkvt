CROSS_TC=/home/llandsmeer/Build/gcc-linaro-7.5.0-2019.12-i686_arm-linux-gnueabihf/bin/arm-linux-gnueabihf
CPPFLAGS=-Wall
LDFLAGS=-lm -Lbuild

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

# pushd Kobo && zip -r ../Release/FBInk-v1.21.0-31-gfdd7300.zip . && popd
# /bin/sh: 1: pushd: not found

clean:
	make -C FBInk clean
	rm -fr build/
