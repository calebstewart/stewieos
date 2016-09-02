#ifndef _STEWIEOS_DIRENT_H_
#define _STEWIEOS_DIRENT_H_

#define DT_UNKNOWN 0
#define DT_REGULAR 1
#define DT_DIRECTORY 2
#define DT_CHARACTER 3
#define DT_BLOCK 4
#define DT_FIFO 5
#define DT_SOCKET 6
#define DT_SYMLINK 7

struct dirent
{
	ino_t d_ino;
	int d_type;
	char d_name[256];
};

struct _DIR;
typedef struct _DIR DIR;

DIR* opendir(const char* name);
int closedir(DIR* dirp);
struct dirent* readdir(DIR* dirp);
void seekdir(DIR* dirp, long loc);
long telldir(DIR* dirp);

#endif 
