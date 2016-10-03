# @author: caleb
# @date: 13 Jun 2016
# Root Makefile used to build all
# components of StewieOS 

.PHONY: toolchain kernel userland all-confirm all
.PHONY: binutils gcc newlib libstdc++ modules libgcc
.PHONY: stewieos clean-all
# These check to see if something is installed, and
# install it if needed.
.PHONY: binutils-installed gcc-installed
.PHONY: newlib-installed

BUILDDIR:=$(abspath ./toolchain/build)
TOOLCHAIN:=$(abspath ./toolchain)
DESTDIR ?= $(abspath ./sysroot)

all:
	@echo "To build the operating system, first build 'toolchain', then build 'stewieos'"

toolchain: binutils gcc newlib libgcc libstdc++

stewieos: kernel modules userland

$(BUILDDIR)/binutils/Makefile:
	cd "$(BUILDDIR)/binutils"; \
	 $(TOOLCHAIN)/binutils/configure --prefix="$(PREFIX)" --target="$(TARGET)" --with-sysroot="$(SYSROOT)" --disable-werror

binutils: $(BUILDDIR)/binutils/Makefile
	$(MAKE) -C "$(BUILDDIR)/binutils" -j 4
	$(MAKE) -C "$(BUILDDIR)/binutils" -j 4 install

$(BUILDDIR)/gcc/Makefile:
	cd  "$(BUILDDIR)/gcc"; \
	 $(TOOLCHAIN)/gcc/configure --prefix="$(PREFIX)" --target="$(TARGET)" --with-sysroot="$(SYSROOT)" --enable-languages=c,c++

gcc: binutils-installed $(BUILDDIR)/gcc/Makefile
	$(MAKE) -C "$(BUILDDIR)/gcc" -j 4 all-gcc
	$(MAKE) -C "$(BUILDDIR)/gcc" -j 4 install-gcc

libgcc: gcc-installed newlib
	$(MAKE) -C "$(BUILDDIR)/gcc" -j 4 all-target-libgcc
	$(MAKE) -C "$(BUILDDIR)/gcc" -j 4 install-target-libgcc

$(BUILDDIR)/newlib/Makefile:
	cd "$(BUILDDIR)/newlib"; \
	$(TOOLCHAIN)/newlib/configure --prefix="$(PREFIX)" --target="$(TARGET)"

newlib: gcc-installed $(BUILDDIR)/newlib/Makefile
	$(MAKE) -C "$(BUILDDIR)/newlib" -j 4
	$(MAKE) -C "$(BUILDDIR)/newlib" -j 4 install
#	cp -ar $(SYSROOT)/$(TARGET)/* $(SYSROOT)/usr/

libstdc++: newlib
	$(MAKE) -C "$(BUILDDIR)/gcc" -j 4 all-target-libstdc++-v3
	$(MAKE) -C "$(BUILDDIR)/gcc" -j 4 install-target-libstdc++-v3

binutils-installed: $(PREFIX)/bin/$(TARGET)-ar
	$(MAKE) binutils

gcc-installed: $(PREFIX)/bin/$(TARGET)-gcc
	$(MAKE) gcc

clean-all:
	$(MAKE) -C "userland" clean
	$(MAKE) -C "modules" clean
	$(MAKE) -C "kernel" clean

userland:
	$(MAKE) -C "userland" all
	$(MAKE) -C "userland" DESTDIR=$(DESTDIR) install

modules: kernel
	$(MAKE) -C "modules" all
	$(MAKE) -C "modules" DESTDIR=$(DESTDIR) install

kernel:
	$(MAKE) -C "kernel" all
	$(MAKE) -C "kernel" DESTDIR=$(DESTDIR) install