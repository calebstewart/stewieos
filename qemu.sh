# runs qemu for StewieOS

cd `dirname $0`
qemu-system-i386 -drive file=stewieos.dd,format=raw -serial stdio -boot c -s $@
