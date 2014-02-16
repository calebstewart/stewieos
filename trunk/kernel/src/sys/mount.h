#ifndef _STEWIEOS_MOUNT_H_
#define _STEWIEOS_MOUNT_H_

// mountflags values
#define MS_RDONLY			0x00000001 // Read only file system
#define MS_NOATIME			0x00000002 // Do not update the access time of files
#define MS_NODEV			0x00000004 // Do not allow access to devices on this filesystem
#define MS_NOEXEC			0x00000008 // Do not allow executable priviledges
#define MS_REMOUNT			0x00000010 // Remount (change mountflags)

int mount(const char* source, const char* target,
	  const char* filesystemtype, unsigned long mountflags,
	  const void* data);
int umount(const char* target);

#endif