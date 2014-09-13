#summary Instructions for Compiling StewieOS
#labels Featured,Phase-Implementation,Phase-Deploy

= Introduction =

StewieOS was built with a GCC cross compiler. In order to build stewieos, you will need to build the cross-compiler yourself. This document outlines the steps needed to reproduce the exact same configuration.


= Details =

==Obtaining the Sources==

First, you need to download the sources for GCC, binutils, and Newlib. They can be found here:

[http://ftpmirror.gnu.org/gcc/gcc-4.9.0/ GCC-4.9.0]

[http://ftpmirror.gnu.org/binutils/ Binutils-2.24]

[https://sourceware.org/newlib/ Newlib-1.2.0]

==Applying Patches==

Within the Subversion repository, there is a folder called patches. This folder contains the patches for each source directory with appropriate names.

First, create a new directory for your extracted sources (I will assume ~/stewieos-cross from now on), then extract the three tar balls above into that directory. Place each patch file within the directory it corresponds to. Open a terminal within that directory and issue the following command for each patch file:

{{{
patch -s -p0 < patch_file.patch
}}}

After that, the source directories are ready for compilation.

==Compiling==

For ease of use later, first define a couple environment variables:

{{{
export TARGET=i586-pc-stewieos
export PREFIX=~/opt/stewieos-cross
}}}

This means we are building a compiler for the stewieos target, and that we will install to ~/opt/stewieos-cross. Next, we need to create the build directories. From within ~/stewieos-cross run:

{{{
mkdir build-binutils build-gcc build-newlib
}}}

Now we can build the targets. First we build and install binutils, and add it to the current path (gcc needs our special binutils to compile).

{{{
cd build-binutils
../binutils-2.24/configure --target=$TARGET --prefix=$PREFIX --disable-werr
make
make install
export PATH=$PATH:$PREFIX/bin
}}}

Next, we build gcc itself:

{{{
cd build-gcc
../gcc-4.9.0/configure --target=$TARGET --prefix=$PREFIX --enable-languages=c
make all-gcc
make install-gcc
make all-target-libgcc
make install-target-libgcc
}}}

Finally, we can build and install newlib

{{{
cd build-newlib
../newlib-1.2.0/configure --target=$TARGET --prefix=$PREFIX
make
make install
}}}

==Usage==

The compiler is completely installed as is, but any newly run shell won't be able to see it. To rectify this, you can either run

{{{
export PATH=$PATH:$HOME/opt/stewieos-cross/bin
}}}

Every time you open a shell and want to use the cross compiler or you can add that command to the file ~/.bash_profile
