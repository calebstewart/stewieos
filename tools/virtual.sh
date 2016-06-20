#!/usr/bin/env bash 
# @Author: Caleb Stewart
# @Date:   2016-06-19 19:33:18
# @Last Modified by:   Caleb Stewart
# @Last Modified time: 2016-06-20 00:00:18
IMAGE_TYPE=vmdk
IMAGE_NAME="stewieos".$IMAGE_TYPE
DEVICE_NAME=/dev/nbd0
PART_NAME=/dev/mapper/nbd0p1
IMAGE_MOUNT=`mktemp -d`
IMAGE_SIZE="1G"
SELF=`basename $0`
RED=`tput setaf 1`
GREEN=`tput setaf 2`
RST=`echo -e "\e[0m"`
UL=`echo -e "\e[4m"`
BLD=`echo -e "\e[1m"`

function info
{
	echo "[${GREEN}info${RST}] $@"
}

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
	mkfs.ext2 "$PART_NAME"
	mount_image
	grub-install --boot-directory="$IMAGE_MOUNT/boot" "$DEVICE_NAME"
	umount_image
}

# Mount the image in our temporary folder
function mount_image
{
	if mount | grep "$PART_NAME" > /dev/null; then
		return
	fi
	mount "$PART_NAME" "$IMAGE_MOUNT"
}

# Sync our system root with the image
function sync_image
{
	rsync -au ./sysroot/* "$IMAGE_MOUNT"
}

# Unmount the system image
function umount_image
{
	if mount | grep "$PART_NAME" > /dev/null; then
		umount "$PART_NAME"
	fi
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
	qemu-system-i386 -drive file="$IMAGE_NAME",format="$IMAGE_TYPE" -serial stdio -boot c -s
}

# Cleanup the system image
# and delete our temp directory
function image_cleanup
{
	umount_image > /dev/null 2>/dev/null
	unload_image > /dev/null 2>/dev/null
	rm -rf $IMAGE_MOUNT > /dev/null 2>/dev/null
}

function print_help
{
	cat <<END
${BLD}USAGE${RST}: $SELF -s|--size ${UL}imagesize${RST} -t|--type ${UL}imagetype${RST} [run|create|sync]...
${BLD}OPTIONS${RST}:
	${UL}-s|--size${RST}:
		The size of the image (when creating)
	${UL}-t|--type${RST}:
		The type of the image (when creating)
${BLD}ACTIONS${RST}:
	${UL}run${RST}:
		execute an emulator to run the virtual image
	${UL}create${RST}:
		create the virtual image (requires root privileges)
	${UL}sync${RST}:
		synchronize the current image with the sysroot directory
END
}

# Make sure that we cleanup before exit
trap image_cleanup EXIT

# Parse command line arguments
ARGS=$(getopt -o s:ht: --long size,help,type -n "$SELF" -- "$@")

# Check for errors
if [ $? != 0 ]; then
	exit 1
fi

# Throw our args into the positional parameters
eval set -- "$ARGS"

# Iterate through the arguments
while true ; do
	case "$1" in
		-h|--help)
			print_help
			exit
			;;
		-s|--size)
			IMAGE_SIZE=$2
			shift ; shift
			;;
		-t|--type)
			IMAGE_TYPE=$2
			shift ; shift
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

for arg; do
	case "$arg" in
		run)
			image_emulate "$EMULATOR"
			;;
		create)
			image_create
			;;
		sync)
			image_sync
			;;
		*)
			error "unrecognized action: $arg"
			exit 1
			;;
	esac
done