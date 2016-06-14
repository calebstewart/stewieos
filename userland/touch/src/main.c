#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char** argv)
{
	for(int i = 1; i < argc; ++i){
		int fd = open(argv[i], O_CREAT, 0777);
		if( fd < 0 ){
			printf("error: unable to touch file %s. error code %d\n", argv[i], errno);
		} else {
			close(fd);
		}
	}
	return 0;
}
