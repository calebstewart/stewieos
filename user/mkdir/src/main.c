#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

int main(int argc, char** argv)
{
	if( argc == 1 ){
		printf("usage: %s directory_names\n", argv[0]);
		return 0;
	}
	
	for( int i = 1; i < argc; ++i)
	{
		if( mknod(argv[i], S_IFDIR | 0777, 0) < 0 ){
			printf("error: %s: unable to create directory. error code %d.\n", argv[i], errno);
			errno = 0;
		} else {
			printf("created directory %s.\n", argv[i]);
		}
	}
	return 0;
}
