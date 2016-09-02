/*
* @Author: Caleb Stewart
* @Date:   2016-06-24 14:47:06
* @Last Modified by:   Caleb Stewart
* @Last Modified time: 2016-07-16 16:36:35
*/
#include "mod.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/message.h>

int modules_load( void )
{

	// Open the modules.conf file
	FILE* modules = fopen("/etc/modules.conf", "r");
	// Allocate a buffer for the line
	char module_line[1024];
	int result = 0;

	if( modules == NULL ){
		syslog(SYSLOG_ERR, "unable to open /etc/modules.conf.");
		return -1;
	}
	
	// Read all the files
	while( fgets(module_line, 1024, modules) != NULL )
	{

		// Strip whitespace at beginning and end
		char* lineptr = module_line;
		while( isspace(*lineptr) ) lineptr++;
		char* endptr = strchr(module_line, '\0')-1;
		for(char* endptr = strchr(module_line, '\0')-1;
			endptr >= lineptr && isspace(*endptr);
			endptr--){
			*endptr = '\0';
		}

		// Treat empty lines like comments
		if( strlen(lineptr) == 0 || lineptr[0] == '#' ){
			continue;
		}
		
		// Attempt to load the module
		result = insmod(lineptr);
		if( result == -1 ){
			syslog(SYSLOG_INFO,"serviced: unable to load module %s: %d", module_line, errno);
			errno = 0;
		} else {
			syslog(SYSLOG_INFO,"serviced: loaded module %s.", module_line);
		}
		
	}

	fclose(modules);

	return 0;
}