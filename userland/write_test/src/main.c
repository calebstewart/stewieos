#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int main(int argc, char** argv)
{
	if( argc == 1 ){
		printf("Please give me a file name!\n");
		return -1;
	}
	
	printf("Opening \"%s\" for writing...\n", argv[1]);
	
	int fd = open(argv[1], O_WRONLY | O_CREAT, 0777);
	if( fd < 0 ){
		printf("Error! Unable to open/create file (errno=%d)!\n", errno);
		return -1;
	}
	
	printf("Writing %d arguments to the file...\n", argc-2);
	
	ssize_t n = 0;
	
	for(int i = 2; i < argc; ++i){
		printf("Writing %s to the file...\n", argv[i]);
		ssize_t result = write(fd, argv[i], strlen(argv[i]));
		if( result < 0 ){
			printf("Error! Write failed after %d bytes written. errno=%d\n", errno);
			close(fd);
			return -1;
		} else if( result < strlen(argv[i]) ){
			printf("Warning! Only wrote %d bytes when %d were requested.\n", result, strlen(argv[i]));
			n += result;
		} else {
			n += result;
		}
		result = write(fd, " ", 1);
		if( result < 0 ){
			printf("Error! Write failed after %d bytes written. errno=%d\n", errno);
			close(fd);
			return -1;
		} else {
			n += result;
		}
	}
	
	// Write an extra new line character for good measure
	ssize_t result = write(fd, "\n", 1);
	if( result < 0 ){
		printf("Error! Write failed after %d bytes written. errno=%d\n", errno);
		close(fd);
		return -1;
	} else {
		n += result;
	}
	
	printf("Successfully wrote %d bytes to the \"%s\"!\n", n, argv[1]);
	
	close(fd);
	
	return 0;
}
