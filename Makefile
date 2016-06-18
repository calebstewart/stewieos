# @author: caleb
# @date: 13 Jun 2016
# Root Makefile used to build all
# components of StewieOS 

.PHONY: toolchain kernel userland all-confirm all
.PHONY: binutils gcc newlib libstdc++ modules libgcc
.PHONY: stewieos

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

gcc: $(BUILDDIR)/gcc/.build_success

gcc: binutils $(BUILDDIR)/gcc/Makefile
	$(MAKE) -C "$(BUILDDIR)/gcc" -j 4 all-gcc
	$(MAKE) -C "$(BUILDDIR)/gcc" -j 4 install-gcc

libgcc: gcc newlib
	$(MAKE) -C "$(BUILDDIR)/gcc" -j 4 all-target-libgcc
	$(MAKE) -C "$(BUILDDIR)/gcc" -j 4 install-target-libgcc

$(BUILDDIR)/newlib/Makefile:
	cd "$(BUILDDIR)/newlib"; \
	$(TOOLCHAIN)/newlib/configure --prefix="$(PREFIX)" --target="$(TARGET)"

newlib: gcc $(BUILDDIR)/newlib/Makefile
	$(MAKE) -C "$(BUILDDIR)/newlib" -j 4
	$(MAKE) -C "$(BUILDDIR)/newlib" -j 4 install
	cp -ar $(SYSROOT)/$(TARGET)/* $(SYSROOT)/usr/

libstdc++: newlib
	$(MAKE) -C "$(BUILDDIR)/gcc" -j 4 all-target-libstdc++-v3
	$(MAKE) -C "$(BUILDDIR)/gcc" -j 4 install-target-libstdc++-v3

userland:
	$(MAKE) -C "userland" all
	$(MAKE) -C "userland" DESTDIR=$(DESTDIR) install

modules: kernel
	$(MAKE) -C "modules" all
	$(MAKE) -C "modules" DESTDIR=$(DESTDIR) install

kernel:
	$(MAKE) -C "kernel" all
	$(MAKE) -C "kernel" DESTDIR=$(DESTDIR) install