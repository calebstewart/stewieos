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
PROJECTS:=kernel
ALLPROJECTS:=$(PROJECTS:%=all-%)
CLEANPROJECTS:=$(PROJECTS:%=clean-%)
INSTALLPROJECTS:=$(PROJECTS:%=install-%)

.PHONY: $(ALLPROJECTS) $(CLEANPROJECTS) $(INSTALLPROJECTS) all clean test install prepare_vhd cleanup_vhd

all: $(ALLPROJECTS)
#@echo "Please build each subsystem one at time. Here is a list of subsystems to build:\n$(PROJECTS)\nE.g. \"make all-'subsystemname'\""

clean: $(CLEANPROJECTS)

install: prepare_vhd $(INSTALLPROJECTS) cleanup_vhd

prepare_vhd:
	losetup /dev/loop0 ./stewieos.vhd
	kpartx -v -a /dev/loop0
	mount /dev/mapper/loop0p1 /mnt
	
cleanup_vhd:
	umount /mnt
	kpartx -v -d /dev/loop0
	losetup -d /dev/loop0

$(ALLPROJECTS):
	$(MAKE) -C $(@:all-%=%) all

$(CLEANPROJECTS):
	$(MAKE) -C $(@:clean-%=%) clean
	
$(INSTALLPROJECTS):
	$(MAKE) -C $(@:install-%=%) install

# Here, you can add dependencies for specific projects
# vhd: kernel