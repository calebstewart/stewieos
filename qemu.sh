# runs qemu for StewieOS

cd `dirname $0`
qemu -hda stewieos.vhd -boot c $@
