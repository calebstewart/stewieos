StewieOS - A hobby OS project
=============================

StewieOS was created as a learning project aimed toward how and why the internals of modern operating systems were designed the way they were. It's been a long road, but I've learned a lot and I hope the code here can help some passerby's at some point.

Structure of the Project
------------------------

This repository not only contains my own work on the kernel, kernel modules, and userspace tools but also the complete toolchain source. In order to keep the toolchain logically seperated (it's quite large), it is connected via git submodules. The submodules are all located within the `toolchain` directory and are forks of their respective toolchain pieces (`binutils`, `gcc`, and `newlib`) with the needed modifications to create a cross compiler/toolchain for StewieOS.

In order to fully download the repository, you should clone using the recursive option:

```
$ git clone --recursive https://github.com/Caleb1994/stewieos
```

If you have already cloned the repository, you can retrieve the submodules with,

```
$ git submodule update --init --recursive
```

Environment - Setup
-------------------

For building and running StewieOS, there will be a few things you need to install.

* Qemu (with `qemu-system-i386`)
* A working build environment (e.g. `gcc`, `binutils`, etc.)
* NASM - Netwide Assembler
* The custom toolchain (have a few :beers: or :coffee: ready; it's a long build)

If you are planning on modifying the toolchain, then you also need:

* Automake 1.11 (_MUST_ have exact version)
* Autoconf 2.64 (_MUST_ have exact version)

When working with `gcc`, `binutils` and `newlib`, you must be very specific about the versioning of these tools. They do not have to be the only version of `automake` and `autoconf` installed. My development machine is Arch Linux, and both of these tools are available with the needed versions (either through AUR or community), but are installed in different locations than the standard autotools. See the "Daily Usage" section for a solution.

Environment - Daily Usage
-------------------------

The first thing to do after entering this directory is source the `init.sh` script into your shell.

```
$ . init.sh
```

or if you prefer,

```
$ source init.sh
```

This is a simple script that sets up the environment to find the custom toolchain we will compile as well as makes sure we are using the correct autotools version. It also adds the `tools` directory to the path, as it contains some useful scripts for working in the environment. The biggest things this script will do is add the custom toolchain to your path as well as alias/add the correct autotools versions to your path. This script assumes the location/name of both `automake-1.11` and `autoconf-2.64`. This is taken from an Arch Linux box, and may not be suited to your configuration. If so, you may need to configure you environment differently without this script. If you do, make sure that `automake --version` and `aclocal --version` report `1.11.x`. Also, `autoconf --version` and `autoreconf --version` must report `2.64.x`.

**NOTE**: If you do not plan on modifying `binutils`, `gcc`, or `newlib`, the presence of the correct versions of `automake` and `autoconf` is not important, as you will never need to reconfigure the projects.

Getting Started
---------------

In order to start working with StewieOS, you must build the toolchain. This is a long process, so I suggest a few :beers: or a glass of :coffee: on hand. It takes a significant amount of time, but should be rather painless. Simply run `make toolchain` in the root of the repository, and you should be on your way to building `binutils`, `gcc`, and `newlib`. It takes somewhere in the range of an hour on my machine to build everything the first time (although, I've never sat to watch it), so good luck.

Once you have built the toolchain, you can run `make stewieos` to build the kernel, kernel modules, and user space applications. These will all be installed within the `sysroot` directory which is effectively the root of the StewieOS disk image. You may add files there to have them installed to the virtual image.

After building StewieOS, you can use the `virtual.sh` script under the `tools` directory to create, update and run a StewieOS virtual machine. See the [tools](./tools) directory for more details.

[1]: https://github.com/Caleb1994/stewieos-kernel
[2]: https://github.com/Caleb1994/stewieos-binutils
[3]: https://github.com/Caleb1994/stewieos-gcc
[4]: https://github.com/Caleb1994/stewieos-newlib