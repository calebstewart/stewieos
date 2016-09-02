#!/bin/sh
# Script to setup path to begin working with the stewieos toolchain

alias automake=automake-1.11
alias aclocal=aclocal-1.11
export AUTOMAKE=automake-1.11
export ACLOCAL=aclocal-1.11
export PREFIX=`readlink -f ./toolchain/root`
export SYSROOT=`readlink -f ./sysroot`
export TARGET=i386-elf-stewieos
export PATH=`pwd`/toolchain/root/bin:$PATH
export PATH=/opt/autoconf/2.64/bin:$PATH
export PATH=`pwd`/tools:$PATH