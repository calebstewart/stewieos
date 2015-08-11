# ACPICA - Intel's Cross Platform ACPI Library

## Description

This Makefile will compile ACPICA into a useable static libary. It simply compiles all of the "components" directory within the ACPICA sources into a static library.  It also copies my personal ACPICA configuration header to include/platform and replaces references to aclinux.h and _LINUX macros with StewieOS variants. Doing this effectively patches the ACPICA sources to recognize the StewieOS kernel. The patches are simple enough to be completed in two `sed` calls and on `cp`. The only file that is changed is acenv.h (to include acstewieos.h). Lastly, it will create a symbolic link to the include directory in StewieOS/kernel/include/acpi (therefore acpi.h will be available as acpi/acpi.h from within the kernel).

## Acquiring ACPICA sources

The Makefile will also download the ACPICA source archive from either Git or a given remote address. By default, it retrieves from Git. If you'd like to change that, you may open the Makefile and set ACPICA_PREFER_ARCHIVE to true, set the ACPICA_ARCHIVE_URL to the URL of the archive download without the archive name, and lastly set ACPICA_ARCHIVE_NAME to the name of the archive. Therefore, $(ACPICA_ARCHIVE_URL)/$(ACPICA_ARCHIVE_NAME) should be the full URL to the archive. If you don't have wget you can change the downloader appropriately, as well.