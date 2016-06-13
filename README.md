StewieOS - Kernel Documentation
===============================

StewieOS is a learning project aimed at understand the underlying mechanics of modern operating systems. It is designed for the x86 architecture and follows a Unix design. At the time of writing, a custom toolchain has been compiled including a C library.

Getting Started - Environment
-----------------------------

If you are looking to build and test the StewieOS kernel (whether for development or not), you need the custom cross compiler/toolchain. The easiest way to obtain this is to go to the [root project](https://github.com/Caleb1994/stewieos) for StewieOS which aggregates all required pieces of the operating system with documentation on how to get your environment ready.

Getting Started - Building and Running
--------------------------------------

As said previously, you must the cross compiler in order to build the kernel. Once you have the cross compiler installed, the kernel is built with a standard Makefile like so:

```
$ make
[... lots of output ...]
```

The Makefile knows how to setup a testing hard disk image, and can do so with the `install_vhd` target.

```
$ sudo make install_vhd
```

This will create a 1GB virtual hard disk (flat binary format) and populate it with the correct partition table, file system, and GRUB installation to boot StewieOS. The script needs `sudo` access in order to create a loop back device and map its partitions for formatting. After this, you may run qemu using the useful qemu script provided. All parameters to the script are passed directly to qemu itself.

```
$ ./qemu.sh
```

That's it! You are now running StewieOS!