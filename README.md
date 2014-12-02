StewieOS
========

StewieOS is a simple hobby 32-bit x86 kernel/operating system. This is my third rewrite to date. The kernel is based on UNIX/POSIX standards although I cannot say I necessaraly follow those standards at all times. It is monolithic, higher-half kernel.It's nothing special at the moment, but I am looking forward to a day when I can boot it up and do something useful with it.

If you would like to see some screenshots or other random things I might upload aside from code, you can check out my [Google Drive folder](https://drive.google.com/folderview?id=0B8V-FEUiNZm4NjU1TWF5UTlWbGM&usp=sharing)

Building
--------

StewieOS is built with a special cross-compiled toolchain. I use GCC-4.9.0, Binutils-2.24, and Newlib 2.1.0. Within the repository, there is folder called "patches" the README file under patches/ will tell you how to build the toolchain. StewieOS also expects a UNIX/Linux environment. It should work in Cygwin under windows, although it has not been tested.

Once the toolchain is built, you can simply call `$ make` within the project root directory and all needed pieces will be compiled. You can then call `# make install`. The install target will not affect your system. It simply installs the kernel to the virtual hard disk "stewieos.dd". This virtual disk needs to already exist, and can be created simply by running "stewieos_make_hdd.sh" under sudo. You need sudo because the script needs to setup a loopback device and partition/format the virtual disk. Once you have stewieos.dd, you can run make install.

Running
-------

Once it is installed, you can use the qemu.sh script to run a virtual machine or call bochs using the provided bochsrc script. Qemu.sh expects qemu-system-i386 to be installed, tells qemu to listen for GDB attachment and also passes all parameters to the script on to qemu itself. The bochsrc is nothing special. It just runs with stewieos.vhd as the hard disk. Although the name is misleading, stewieos.vhd is not actually a VirtualBox Compatable VHD. It is a flat hard disk image. You can either use a loopback device or look online about how to use a flat image with other emulators.
