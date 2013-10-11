#ifndef _STEWIEOS_MOUNT_H_
#define _STEWIEOS_MOUNT_H_

// mountflags values
#define MF_RDONLY			0x00000001 // Read only file system
#define MF_NOATIME			0x00000002 // Do not update the access time of files
#define MF_NODEV			0x00000004 // Do not allow access to devices on this filesystem
#define MF_NOEXEC			0x00000008 // Do not allow executable priviledges
#define MF_REMOUNT			0x00000010 // Remount (change mountflags)

int mount(const char* source, const char* target,
	  const char* filesystemtype, unsigned long mountflags,
	  const void* data);
int umount(const char* target);

#endif