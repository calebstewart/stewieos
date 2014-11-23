#include <stdio.h>
#include <dirent.h>
#include <errno.h>

int main(int argc, char** argv)
{
	const char* path = "./";
	struct dirent* dirent = NULL;
	
	if( argc == 2 ){
		path = argv[1];
	}
	
	DIR* dirp = opendir(path);
	if( dirp == NULL ){
		printf("error: unable to open directory: %d\n", errno);
		return -1;
	}
	
	while( (dirent = readdir(dirp)) != NULL )
	{
		printf("%s\n", dirent->d_name);
	}
	
	closedir(dirp);
	
	return 0;
}
