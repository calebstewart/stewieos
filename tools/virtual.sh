#!/usr/bin/env bash 
# @Author: Caleb Stewart
# @Date:   2016-06-19 19:33:18
# @Last Modified by:   Caleb Stewart
# @Last Modified time: 2016-07-16 16:25:27

# Default image type
IMAGE_TYPE=vmdk
# Virtual disk image name
IMAGE_NAME="stewieos".$IMAGE_TYPE
# The NBD device name to use
DEVICE_NAME=/dev/nbd0
# The file path/name of the partition mapping (from kpartx)
PART_NAME=/dev/mapper/nbd0p1
# The mount directory
# This will be cleaned up before exit
IMAGE_MOUNT=`mount | grep "$PART_NAME" | awk '{ print $3 }'` #`mktemp -d`
# The size of the virtual disk image
IMAGE_SIZE="1G"
# Name of this script
SELF=`basename $0`
# Useful styles/colors
RED=`tput setaf 1`
GREEN=`tput setaf 2`
RST=`echo -e "\e[0m"`
UL=`echo -e "\e[4m"`
BLD=`echo -e "\e[1m"`
# Default options for QEMU emulator
QEMU_OPTIONS="-drive file="$IMAGE_NAME",format="$IMAGE_TYPE" -serial stdio -boot c"

# Print a formatted information message
function info
{
	echo "[${GREEN}info${RST}] $@"
}

# Print a formatted error message
function error
{
	echo "[${RED}error${RST}] $@" >&2
}

# Create the disk image
function create_image
{
	qemu-img create -f "$IMAGE_TYPE" "$IMAGE_NAME" "$IMAGE_SIZE"
	chown `logname`:`logname` "$IMAGE_NAME"
}

# Load the image as a block device into the system
function load_image
{
	qemu-nbd -c "$DEVICE_NAME" "$IMAGE_NAME"
	kpartx -a "$DEVICE_NAME"
	# Sometimes the script goes to fast
	# and can't see the new partitions
	sleep 0.1
}

# Unload the image from the system
function unload_image
{
	kpartx -d "$DEVICE_NAME"
	qemu-nbd -d "$DEVICE_NAME"
}

# Remove, then load the image
function reload_image
{
	unload_image
	load_image
}

# After loading the image, format it for
# use as a system disk image
function format_image
{
	FDISK_COMMANDS="o\n\nn\np\n\n\n\na\nw\nq\ny"
	echo -e $FDISK_COMMANDS | fdisk "$DEVICE_NAME" >/dev/null 2>/dev/null
	reload_image
	mkfs.ext2 -r 0 -t ext2 -L "StewieOS" "$PART_NAME"
	mount_image
	grub-install --boot-directory="$IMAGE_MOUNT/boot" "$DEVICE_NAME"
	umount_image
}

# Mount the image in our temporary folder
function mount_image
{
	if [ "$IMAGE_MOUNT" == "" ]; then
		IMAGE_MOUNT=`mktemp -d`
	else
		return
	fi
	mount "$PART_NAME" "$IMAGE_MOUNT"
	sleep 0.1
}

# Sync our system root with the image
function sync_image
{
	rsync -au ./sysroot/* "$IMAGE_MOUNT"
}

# Unmount the system image
function umount_image
{
	if [ "$IMAGE_MOUNT" == "" ]; then
		return
	fi
	umount $PART_NAME && rmdir $IMAGE_MOUNT
}

# Create and setup the virtual image
function image_create
{
	info "creating image..."
	create_image
	load_image
	info "formatting image..."
	format_image
	info "mounting image..."
	mount_image
	info "synchronizing system root..."
	sync_image
	info "cleaning up..."
	umount_image
	unload_image
}

# Synchronize the image with the sysroot directory
function image_sync
{
	info "mounting image..."
	load_image
	mount_image
	info "synchronizing system root..."
	sync_image
	info "cleaning up..."
	umount_image
	unload_image
}

# Spin up the emulator based on the image
function image_emulate
{
	info "starting qemu emulator..."
	info "connecting virtual serial port to terminal..."
	qemu-system-i386 $QEMU_OPTIONS
}

# Cleanup the system image
# Dump everything to /dev/null, because these commands may
# fail (and it's okay). image_cleanup is always called by `trap`.
function image_cleanup
{
	umount_image > /dev/null 2>/dev/null && unload_image > /dev/null 2>/dev/null
}

# Print the help message for this script
function print_help
{
	cat <<END
${BLD}USAGE${RST}: $SELF -s|--size ${UL}imagesize${RST} -t|--type ${UL}imagetype${RST} [run|create|sync]...

	This command is used to manage the StewieOS virtual disk. You can create,
	update and emulate a virtual machine disk image using this script. Multiple
	actions may be run in one call, and one action can be run multiple times,
	although I do not know why you would want to.

${BLD}OPTIONS${RST}:
	${UL}-s|--size${RST}:
		The size of the image (when creating)
	${UL}-t|--type${RST}:
		The type of the image (when creating)
	${UL}-d|--debug${RST}:
		Run VM in debug mode (wait for connection)
${BLD}ACTIONS${RST}:
	${UL}run${RST}:
		execute an emulator to run the virtual image
	${UL}create${RST}:
		create the virtual image (requires root privileges)
	${UL}sync${RST}:
		synchronize the current image with the sysroot directory
END
}

# Parse command line arguments
ARGS=$(getopt -o ds:ht: --long size,help,type -n "$SELF" -- "$@")

# Check for errors
if [ $? != 0 ]; then
	exit 1
fi

# Throw our args into the positional parameters
eval set -- "$ARGS"

# Iterate through the arguments
while true ; do
	case "$1" in
		# Print the help message and exit
		-h|--help)
			print_help
			exit
			;;
		# Set the size of the image (when creating)
		-s|--size)
			IMAGE_SIZE=$2
			shift ; shift
			;;
		# Set the type of the image (when creating)
		-t|--type)
			IMAGE_TYPE=$2
			shift ; shift
			;;
		# Turn on debugging for the VM (start and wait)
		-d|--debug)
			QEMU_OPTIONS="$QEMU_OPTIONS -s -S"
			shift
			;;
		--)
			shift ; break
			;;
		*)
			error "unknown error."
			exit 1
			;;
	esac
done

# There should be an action specified
if [ "$#" == "0" ]; then
	error "no action specified!"
	print_help
	exit 1
fi

# Execute all the actions
for arg; do
	case "$arg" in
		# Execute the emulator with the image
		run)
			# Make sure that we cleanup before exit
			trap image_cleanup EXIT
			image_emulate "$EMULATOR"
			;;
		# Create the image
		create)
			# Make sure that we cleanup before exit
			trap image_cleanup EXIT
			image_create
			;;
		# Synchronize the image with the sysroot directory
		sync)
			# Make sure that we cleanup before exit
			trap image_cleanup EXIT
			image_sync
			;;
		mount)
			load_image
			mount_image
			if [ "$?" -ne "0" ]; then
				error "unable to mount image!"
				error "removing temporary directory."
				rmdir $IMAGE_MOUNT
			else
				info "image mounted at $IMAGE_MOUNT."
			fi
			;;
		umount)
			umount_image && unload_image 
			if [ "$?" -ne "0" ]; then
				error "unable to unmount image!"
			else
				info "image unmounted."
				info "temporary directory removed ($IMAGE_MOUNT)."
			fi
			;;
		*)
			error "unrecognized action: $arg"
			exit 1
			;;
	esac
done