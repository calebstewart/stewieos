# StewieOS - Development Environment Tools

All tools here expect to be run from the root of the repository.

## `virtual.sh` - StewieOS Virtual Machine Manager

The `virtual.sh` script has the ability to create, update and run a virtual machine based on the most recent build of StewieOS.

### Usage Examples

* Create a 3GB virtual hard disk of type VMDK
```
# virtual.sh create
```

* Create a 10GB virtual hard disk of type QCOW2
```
# virtual.sh -s 10GB -t qcow2 create
```

* Synchronize the image with the files in `sysroot`
```
# virtual.sh sync
```

* Start the QEMU emulator
```
$ virtual.sh run
```

* Synchronize the image and start the emulator
```
$ virtual.sh sync run
```
