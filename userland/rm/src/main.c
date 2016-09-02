#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char** argv)
{
	if( argc == 1 ){
		printf("usage: %s FILENAMES...\ndescription:\n\tremove (unlink) the specified files.\n");
		return 0;
	}

	for(int i = 1; i < argc; ++i){
		if( unlink(argv[i]) < 0 ){
			printf("error: %s: failed to remove: %s (%d)\n", argv[i], strerror(errno), errno);
			errno = 0;
		}
	}

	return 0;
}
