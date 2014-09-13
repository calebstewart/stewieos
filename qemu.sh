# runs qemu for StewieOS

cd `dirname $0`
qemu-system-i386 -hda stewieos.vhd -boot c -s $@
