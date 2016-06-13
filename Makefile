#====================================================================
#
# Author: Caleb Stewart
# Date: 10 September 2013
# Prupose: A top-level make file to build the OS, toolchain, initrd,
# :		etc...
#
# ===================================================================
# = Change Log                                                      |
# ===================================================================
# =    Date    |    Programmer   |    Description                   |
# ===================================================================
# = 10SEP2013  | Caleb Stewart   | Initial Creation                 |
# =            |                 |                                  |
# ===================================================================
# = 15SEP2013  | Caleb Stewart   | Edited so you have to compile    |
# =            |                 | each project seperately          |
# ===================================================================
#
#====================================================================


# This is where you can add project directories to the source
# kernel should come first here!
PROJECTS:=kernel modules user
ALLPROJECTS:=$(PROJECTS:%=all-%)
CLEANPROJECTS:=$(PROJECTS:%=clean-%)
INSTALLPROJECTS:=$(PROJECTS:%=install-%)
DEVICE=/dev/null

.PHONY: $(ALLPROJECTS) $(CLEANPROJECTS) $(INSTALLPROJECTS) all clean test install_vhd prepare_vhd cleanup_vhd fix_vhd perpare_device cleanup_device install_device
.PHONY: mount umount install_files create_vhd fsck do_fsck

# Exported variables for project makefiles
export TOOLCHAIN_LOCATION:=$(HOME)/opt/stewieos-cross
export KERNEL_DIR:=$(abspath ./kernel)
export MODULES_DIR:=$(abspath ./modules)
export STEWIEOS_DIR:=$(abspath ./)
export STEWIEOS_CURRENT:=$(abspath ./stewieos-current)
export STEWIEOS_IMAGE:=$(abspath ./stewieos.dd)
export STEWIEOS_BIN:=/mnt/bin
export STEWIEOS_OPT:=/mnt/opt
export STEWIEOS_ROOT:=/mnt
export CC:=i386-elf-stewieos-gcc
export LD:=$(CC)
export ASM:=nasm

all: $(ALLPROJECTS)

clean: $(CLEANPROJECTS)

install_vhd: stewieos.dd prepare_vhd install_files $(INSTALLPROJECTS) cleanup_vhd

mount: prepare_vhd

umount: cleanup_vhd

fsck: prepare_vhd do_fsck fix_vhd

do_fsck:
	umount /mnt
	-e2fsck -f /dev/mapper/loop0p1

stewieos.dd: 
	./stewieos_make_hdd.sh

prepare_vhd:
	losetup /dev/loop0 $(STEWIEOS_IMAGE)
	kpartx -v -a /dev/loop0
	sleep .1
	mount /dev/mapper/loop0p1 /mnt
	
cleanup_vhd:
	umount /mnt
	kpartx -v -d /dev/loop0
	losetup -d /dev/loop0
	
install_device: prepare_device install_files $(INSTALLPROJECTS) cleanup_device
	
prepare_device:
	mount $(DEVICE) /mnt

cleanup_device:
	umount /mnt

#Sometimes cleanup_vhd fails, so we need this to make the fix a little easier
#I don't know why it fails, but in the mean time this works.
fix_vhd:
	kpartx -v -d /dev/loop0
	losetup -d /dev/loop0

install_files:
	cp -R -u ./fs_root/* /mnt/

$(ALLPROJECTS):
	$(MAKE) -C $(@:all-%=%) all

$(CLEANPROJECTS):
	$(MAKE) -C $(@:clean-%=%) clean
	
$(INSTALLPROJECTS):
	$(MAKE) -C $(@:install-%=%) install

# Here, you can add dependencies for specific projects
# vhd: kernel
