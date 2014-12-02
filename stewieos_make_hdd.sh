#!/bin/bash
# This script will create a hard disk image for use with stewieos.
# It requires root permissions, because it sets up a loopback device, partitions the device
# and install an ext2 filesystem on the device.

# All done! Cleanup mapping and loop device
function cleanup {
	echo "Cleaning up..."
	umount /dev/mapper/loop0p1
	kpartx -d /dev/loop0
	losetup -d /dev/loop0
}

# Check for root permissions
if [ "$(id -u)" != "0" ]; then
	echo "error: you must be root!"
	exit 1
fi

# Create the blank image (1GB in size)
echo "Dumping a 1GB Virtual Disk to ./stewieos.dd..."
dd if=/dev/zero of=./stewieos.dd bs=1073741824 count=1 > /dev/null 2> /dev/null
if [ $? -ne 0 ]; then
	echo "error: unable to create disk image: $?"
	exit 1
fi
chmod ug+rw ./stewieos.dd
chown caleb:caleb ./stewieos.dd

# Create a loopback device to access this image
echo "Setting up loopback device..."
losetup /dev/loop0 ./stewieos.dd
if [ $? -ne 0 ]; then
	echo "error: unable to setup loopback device: $?"
	rm stewieos.dd
	exit 1
fi

# This creates a new parition with fdisk by sending the commands you usually would graphically
# It's REALLY noisy so we redirect to null
echo "Creating a primary partition..."
echo -e "o\nn\np\n1\n\n\nw\nq\n" | fdisk /dev/loop0 > /dev/null 2> /dev/null
# if [ $? -ne 0 ]; then
# 	echo "error: unable to create primary partition: $?"
# 	losetup -d /dev/loop0
# 	exit
# fi

# Map the partitions into the system
echo "Mapping new partition into system..."
kpartx -a /dev/loop0
if [ $? -ne 0 ]; then
	echo "error: unable to map partition: $?"
	losetup -d /dev/loop0
	rm stewieos.dd
	exit 1
fi

# Format the partition (it's noisy so we redirect to null)
echo "Creating EXT2 Filesystem on Device..."
mkfs.ext2 -L "StewieOS" -t ext2 /dev/mapper/loop0p1 > /dev/null 2> /dev/null
if [ $? -ne 0 ]; then
	echo "error: unable to create filesystem: $?"
	kpartx -d /dev/loop0
	losetup -d /dev/loop0
	rm stewieos.dd
	exit 1
fi

# Mount the new filesystem
echo "Mounting new filesystem..."
mount /dev/mapper/loop0p1 /mnt
if [ $? -ne 0 ]; then
	echo "error: unable to mount: $?"
	kpartx -d /dev/loop0
	losetup -d /dev/loop0
	rm stewieos.dd
	exit 1
fi
# Install GRUB
echo "Installing GRUB..."
grub-install --boot-directory=/mnt/boot /dev/loop0
if [ $? -ne 0 ]; then
	echo "error: unable to install grub: $?"
	cleanup
	rm stewieos.dd
	exit 1
fi

# Call the cleanup routine
cleanup