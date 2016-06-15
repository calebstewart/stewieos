# @author: caleb
# @date: 13 Jun 2016
# Root Makefile used to build all
# components of StewieOS 

.PHONY: toolchain kernel userland all-confirm all
.PHONY: binutils gcc newlib libstdc++ modules

BUILDDIR:=$(abspath ./toolchain/build)
TOOLCHAIN:=$(abspath ./toolchain)
DESTDIR ?= $(abspath ./sysroot)

all:
	@echo "Building all components can take considerable time."
	@echo "If you are sure that's what you want, run 'make all-confirm'"

all-confirm: toolchain kernel modules userland

toolchain: binutils gcc newlib libstdc++

$(BUILDDIR)/binutils/Makefile:
	cd "$(BUILDDIR)/binutils"; \
	 $(TOOLCHAIN)/binutils/configure --prefix="$(PREFIX)" --target="$(TARGET)" --with-sysroot="$(SYSROOT)" --disable-werror

binutils: $(BUILDDIR)/binutils/Makefile
	$(MAKE) -C "$(BUILDDIR)/binutils" -j 4
	$(MAKE) -C "$(BUILDDIR)/binutils" -j 4 install

$(BUILDDIR)/gcc/Makefile:
	cd  "$(BUILDDIR)/gcc"; \
	 $(TOOLCHAIN)/gcc/configure --prefix="$(PREFIX)" --target="$(TARGET)" --with-sysroot="$(SYSROOT)" --enable-languages=c,c++

gcc: binutils
	$(MAKE) -C "$(BUILDDIR)/gcc" -j 4 all-gcc all-target-libgcc
	$(MAKE) -C "$(BUILDDIR)/gcc" -j 4 install-gcc install-target-libgcc

$(BUILDDIR)/newlib/Makefile:
	cd "$(BUILDDIR)/newlib"; \
	 $(TOOLCHAIN)/newlib/configure --prefix="$(PREFIX)" --target="$(TARGET)"

newlib: gcc
	$(MAKE) -C "$(BUILDDIR)/newlib" -j 4
	$(MAKE) -C "$(BUILDDIR)/newlib" -j 4 DESTDIR="$(SYSROOT)" install

libstdc++: newlib
	$(MAKE) -C "$(BUILDDIR)/gcc" -j 4 all-target-libstdc++-v3
	$(MAKE) -C "$(BUILDDIR)/gcc" -j 4 install-target-libstdc++-v3

userland: newlib
	$(MAKE) -C "userland" all
	$(MAKE) -C "userland" DESTDIR=$(DESTDIR) install

modules: kernel
	$(MAKE) -C "modules" all
	$(MAKE) -C "modules" DESTDIR=$(DESTDIR) install

kernel: newlib
	$(MAKE) -C "kernel" all
	$(MAKE) -C "kernel" DESTDIR=$(DESTDIR) install