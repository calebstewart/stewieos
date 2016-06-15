StewieOS - A hobby OS project
=============================

StewieOS was created as a learning project aimed toward how and why the internals of modern operating systems were designed the way they were. It's been a long road, but I've learned a lot and I hope the code here can help some passerby's at some point.

**NOTE**: This repository is meant to consolidate the StewieOS project pieces (e.g. toolchain, kernel, and hopefully userspace at somepoint). It is mostly for documentation purposes and to join multiple large projects into one without merging their source code. The documentation here is a work in progress, and should not be followed without some intelligent sanity checks throughout. Good luck. I'm not going to lie to you. Getting the initial environment setup can be a little time consuming and frustrating, but once it is set up, the usage is fairly simple.

Structure of the Project
------------------------

This repository is more a hub, than the actual code. It utilizes Git Submodules to include the real projects. The needed projects are:

* The StewieOS [Kernel][1]
* A custom [Binutils][2] port.
* A custom [GCC][3] port.
* A custom [Newlib][4] port.

With these four projects, you can begin working on (or just compiling) StewieOS. It's a lot of source code to download and compile due to the inclusion of GCC and Binutils, so I'd grab a quick :beer: (or maybe :coffee: depending on the time of day) and get ready to sit back and watch black and white text scroll. 

Environment - Setup
-------------------

For building and running StewieOS, there will be a few things you need to install.

* Qemu (with `qemu-system-i386`)
* A working build environment (e.g. `gcc`, `binutils`, etc.)
* NASM - Netwide Assembler
* The custom toolchain (have a few :beers: or :coffee: ready...)
* Automake 1.11 (_MUST_ have exact version)
* Autoconf 2.64 (_MUST_ have exact version)

The most important thing there is `automake` and `autoconf`. When working with `gcc`, `binutils` and `newlib`, you must be very specific about the versioning of these tools. They do not have to be the only version installed. My development machine is Arch Linux, and both of these tools are available with those versions (either through AUR or community), but are installed in different locations than the standard autotools. See the "Daily Usage" section for a solution.

For instructions on building the StewieOS custom toolchain, see the [toolchain build](./toolchain/build) directory. Make sure you source the `init.sh` script before beginning the toolchain installation.

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

This is a simple script that sets up the environment to find the custom toolchain we will compile as well as makes sure we are using the correct autotools version. The biggest things this script will do is add the custom toolchain to your path as well as alias/add the correct autotools versions to your path. This script assumes the location/name of both `automake-1.11` and `autoconf-2.64`. This is taken from an Arch Linux box, and may not be suited to your configuration. If so, you may need to configure you environment differently without this script. If you do, make sure that `automake --version` and `aclocal --version` report `1.11.x`. Also, `autoconf --version` and `autoreconf --version` must report `2.64.x`.

**NOTE**: If you do not plan on modifying `binutils`, `gcc`, or `newlib`, the presence of the correct versions of `automake` and `autoconf` is not important, as you will never need to reconfigure the projects.

Getting Started
---------------

Once you have visited the build directories and have a working toolchain, you are ready to create applications for or modify StewieOS. If you are interested in customizing or adding to the kernel, you may visit its repository documentation. Each submodule here should have its own documentation on workflow.

If you simply want to run the kernel and see the OS in action, you should visit the [kernel][1] submodule as it will have instructions on building the kernel, and creating a hard disk image for testing.

[1]: https://github.com/Caleb1994/stewieos-kernel
[2]: https://github.com/Caleb1994/stewieos-binutils
[3]: https://github.com/Caleb1994/stewieos-gcc
[4]: https://github.com/Caleb1994/stewieos-newlib