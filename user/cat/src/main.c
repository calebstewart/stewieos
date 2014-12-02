#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char** argv)
{
	char c = 0;
	
	if( argc == 1 ){
		return 0;
	}
	
	for(int f = 1; f < argc; ++f)
	{
		int fd = open(argv[f], O_RDONLY);
		while( read(fd, &c, 1) == 1 ){
			printf("%c", c);
		}
		close(fd);
	}
	
	return 0;
}
