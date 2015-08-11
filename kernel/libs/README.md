# Kernel Static Libraries

This folder houses external libraries which are linked directly to the kernel. This is similar to a kernel module, but it is still linked at compile time. The best example for this is ACPICA. Go checkout the README.md for ACPICA for more information there.

Basically, this is for code which resides in the kernel, but was written and/or maintained by someone else. Add your project name (must be the same as the directory name) to the list within the Makefile, and then create a Makefile for your project. The `make all` target should output a lib[yourprojectname].a file into this directory. It will be automatically linked into the kernel after that. No methods are automatically called, so the code must already be referenced from within the kernel.
