# This Makefile will automatically attempt to retrieve the ACPICA sources
# via the Git Repo. If you prefer to use the archive, you must specify an
# archive path (you can just change the date), and set ACPICA_PREFER_ARCHIVE
# to true.
ACPICA_GIT_REPO:=https://github.com/acpica/acpica.git
GIT:=git

# Uncomment to use the archive
ACPICA_PREFER_ARCHIVE:=true
ACPICA_ARCHIVE_NAME:=acpica-unix-20150717.tar.gz
ACPICA_ARCHIVE_URL:=https://acpica.org/sites/acpica/files
DOWNLOADER:=wget -N
TAR:=tar
#DOWNLOADER:=curl -O

ACPICA_DIR:=acpica

# The output name for the library
LIBDIR=../
LIB:=$(LIBDIR)/libacpica.a

ACENV:=$(ACPICA_DIR)/source/include/platform/acenv.h

# Compile Options
CFLAGS:=-I$(ACPICA_DIR)/source/include -I$(KERNEL_DIR)/include -D _STEWIEOS -D __KERNEL__ -ffreestanding -Wall -fno-stack-protector -Wstrict-prototypes -std=gnu99 -g

# Objects/Sources
SRCS=$(wildcard $(ACPICA_DIR)/source/components/*/*.c)
OBJS=$(SRCS:%.c=%.o)

.PHONY: all clean acpica_patch

all: $(LIB)

clean:
	rm $(LIB) $(OBJS)

$(LIB): $(ACPICA_DIR) acpica_patch $(OBJS)
	$(AR) rcs $(LIB) $(OBJS)
	rm -f $(KERNEL_DIR)/include/acpi
	ln -s $(shell readlink -f $(ACPICA_DIR)/source/include) $(KERNEL_DIR)/include/acpi

acpica_patch:
	cp acstewieos.h $(ACPICA_DIR)/source/include/platform/
	sed -i 's/aclinux/acstewieos/' $(ACENV)
	sed -i 's/_LINUX/_STEWIEOS/' $(ACENV)
	
$(ACPICA_DIR):
ifeq ($(ACPICA_PREFER_ARCHIVE), true)
	$(DOWNLOADER) $(ACPICA_ARCHIVE_URL)/$(ACPICA_ARCHIVE_NAME)
	tar xvfs $(ACPICA_ARCHIVE_NAME)
	mv $(ACPICA_ARCHIVE_NAME:%.tar.gz=%) acpica
else
	$(GIT) clone $(ACPICA_GIT_REPO)
endif