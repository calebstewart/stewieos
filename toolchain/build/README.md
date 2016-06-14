StewieOS - Toolchain Building
=============================

StewieOS comes with a toolchain complete with custom ports of `gcc`, `binutils`, and `newlib`. Most syscalls are implemented, and those that aren't have sane defaults. This document serves as an overview of how to build the toolchain and setup your system for StewieOS development.

You should have source the `init.sh` script from the root of the repository before attempting these commands. Also, all paths are assumed relative to the root of the repository unless otherwise noted.

Building Binutils
-----------------

Binutils provides us with simple binary utilities such as `ld`, and `ar` as well as libraries to interface with binary files. This is prerequisite for `gcc`. There should not be any needed modifications to `binutils`, so building is simple:

```
$ cd build/binutils
$ ../../binutils/configure --prefix=$PREFIX --target=$TARGET --with-sysroot=$SYSROOT --disable-werror
$ make -j 4
$ make install
```

Building GCC
------------

**NOTE**. This will take some time. Grab a :beer:.

GCC provides `C` and `C++` compiler support as well as the `libstdc++` library (later). We first have to build `gcc` itself and then after building `newlib`, we will come back to build and install `libstdc++`, as it depends on `libc`.

```
$ cd build/gcc
$ ../../gcc/configure --prefix="$PREFIX" --target="$TARGET" --with-sysroot="$SYSROOT" --enable-languages=c,c++
$ make -j 4 all-gcc all-target-libgcc
$ make install-gcc install-target-libgcc
```

Building Newlib
---------------

Newlib provides the `C` library implementation for the system. Included with newlib is the system call implementation specific to StewieOS.

If you would like to make modifications, you can look under `newlib/newlib/libc/sys/stewieos`. System header files can be added under the `sys` subdirectory. Any existing source files can be simply modified and recompiled.

If you need to add a new source file, you must add it to both `EXTRA_lib_a_sources` and `extra_objs` within `Makefile.am`. After modifying `Makefile.am`, you should run `autoconf` within `newlib/newlib/libc/sys` and `autoreconf` within `newlib/newlib/libc/sys/stewieos` in order to update the `configure` script appropriately. 

To configure and build newlib, simply run:

```
$ cd build/newlib
$ ../../newlib/configure --prefix="$PREFIX" --target="$TARGET"
$ make -j 4
$ make DESTDIR="$SYSROOT" install
$ cp -ar "$SYSROOT/usr/$PREFIX/*" "$SYSROOT/usr/"
```

Building libstdc++
------------------

We are now ready to build our last piece: `libstdc++`. It is pretty straightforward, and nearly identical to all the other builds.

```
$ cd build/gcc
$ make -j 4 all-target-libstdc++-v3
$ make install-target-libstdc++-v3
```

Toolchain Usage
---------------

Now that your toolchain is installed, you can simply call `i386-elf-stewieos-gcc` or `i386-elf-stewieos-g++` in order to compile executables for StewieOS.