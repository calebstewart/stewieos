/*
* @Author: Caleb Stewart
* @Date:   2016-06-24 14:47:20
* @Last Modified by:   Caleb Stewart
* @Last Modified time: 2016-07-17 17:47:55
*/
#include "svc.h"
#include <json-parser/json.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/message.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>

int json_array_len(json_value* array);
json_value* json_array_get(json_value* array, int idx);
json_value* json_object_get(json_value* object, const char* name);
char* read_file(const char* name, long* pLength);
json_value* _service_open(const char* name);
int _service_call(const char* name, json_value* manifest,
	const char* action);
json_value* _service_get_action(json_value* manifest, const char* action);

int service_running(const char* name)
{
	char pidfile_name[512];
	FILE* pidf;
	pid_t pid;

	// Create the file name
	snprintf(pidfile_name, 512, "/etc/services/%s/pid", name);

	// Attempt to open the pid file
	pidf = fopen(pidfile_name, "r");
	// Pid file doesn't exist, the service is stopped.
	if( pidf )
	{
		fscanf(pidf, "%d", &pid);
		fclose(pidf);
		errno = 0;
		if( waitpid(pid, NULL, WNOHANG) < 0 && errno != ECHILD ){
			return 1;
		} else if( errno != 0 ){
			unlink(pidfile_name);
			//remove(pidfile_name);
			return 0;
		}
	}

	// Not running!
	return 0;
}

int service_start(const char* name)
{
	json_value* manifest;
	pid_t pid = 0;
	json_value* binary;
	char* argv[2] = { NULL, NULL };
	FILE* pidfile= NULL;
	char pidfile_name[512];

	// The service is already running
	if( service_running(name) ){
		return 0;
	}

	// Open the service manifest
	manifest = service_open(name);
	if( manifest == NULL ){
		errno = EINVAL;
		return -1;
	}

	// Grab the binary name
	binary = json_object_get(manifest, "binary");
	if( binary == NULL ){
		errno = EINVAL;
		return -1;
	}

	if( binary->type != json_string ){
		errno = EINVAL;
		return -1;
	}

	// Fork the process
	pid = fork();

	// Unable to fork process
	if( pid < 0 ){
		return -1;
	}

	// We are the parent all is good!
	if( pid > 0 ){
		return 0;
	}

	// Write the pid to a file
	snprintf(pidfile_name, 512, "/etc/services/%s/pid", name);
	pidfile = fopen(pidfile_name, "w");
	fprintf(pidfile, "%d\n", getpid());
	fclose(pidfile);

	argv[0] = binary->u.string.ptr;

	// Attempt to execute the service
	if( execve(argv[0], argv, environ) < 0 ){
		syslog(SYSLOG_ERR, "%s: unable to execute binary: %d", name, errno);
	}

	// We failed.
	// Remove the PID file, and exit.
	remove(pidfile_name);
	exit(-1);

	// This should never happen.
	// Must. Please. Compiler...
	return 0;
}

int service_stop(const char* name)
{
	json_value* manifest;
	FILE* pidfile;
	char pidfile_name[512];
	pid_t pid;

	// Format the pid file name
	snprintf(pidfile_name, 512, "/etc/services/%s/pid", name);

	// Open the PID file
	pidfile = fopen(pidfile_name, "r");

	// Service wasn't running!
	if( pidfile == NULL ){
		return 0;
	}

	// Read the pid
	fscanf(pidfile, "%d", &pid);

	// Close the file
	fclose(pidfile);

	// Remove the pid file
	remove(pidfile_name);

	// Kill the process
	kill(pid, SIGKILL);

	return 0;
}

json_value* service_open(const char* name)
{
	char manifest_name[512], *manifest_data;
	char json_error[json_error_max];
	long manifest_data_len = 0;
	json_value* manifest = NULL;
	json_settings settings = {
		.settings = json_enable_comments
	};

	// Create the path name
	snprintf(manifest_name, 512, "/etc/services/%s/manifest.json", name);

	// Read in the manifest
	manifest_data = read_file(manifest_name, &manifest_data_len);
	if( manifest_data == NULL ){
		syslog(SYSLOG_INFO,"serviced: %s: error: cannot open service manifest.", name);
		return NULL;
	}

	// Parse the JSON
	manifest = json_parse_ex(&settings,
		manifest_data, manifest_data_len, json_error);
	free(manifest_data);
	if( manifest == NULL ){
		syslog(SYSLOG_INFO,"serviced: %s: error: malformed service manifest.", name);
		syslog(SYSLOG_INFO,"serviced: %s: error: json: %s", name, json_error);
		return NULL;
	}

	// Make sure the manifest is an object
	if( manifest->type != json_object ){
		syslog(SYSLOG_INFO,"serviced: %s: error: root node should be an object.", name);
		json_value_free(manifest);
		return NULL;
	}

	return manifest;
}

json_value* service_get_action(json_value* manifest, const char* action_name)
{
	json_value* implements = json_object_get(manifest, "implements");

	if( implements == NULL ){
		syslog(SYSLOG_INFO,"serviced: error: missing implements list.");
		return NULL;
	}

	// Look for the matching action
	for( int i = 0; i < json_array_len(implements); ++i){
		json_value* value = json_array_get(implements, i);
		if( value->type == json_string ){
			if( strncmp(value->u.string.ptr, action_name, value->u.string.length) == 0 ){
				return value;
			}
		} else if( value->type == json_object ){
			json_value* name_value = json_object_get(value, "name");
			if( name_value->type != json_string ){
				syslog(SYSLOG_INFO,"serviced: error: malformed implements list.");
				return NULL;
			}
			if( strncmp(name_value->u.string.ptr, action_name, name_value->u.string.length) == 0 ) {
				return value;
			}
		} else {
			syslog(SYSLOG_INFO,"serviced: error: malformed implements list.");
			return NULL;
		}
	}
}

// int _service_call(const char* name, json_value* manifest, const char* action_name)
// {
// 	char script_path[512];
// 	json_value* script_name = json_object_get(manifest, "name");
// 	json_value* action = _service_get_action(manifest, action_name);

// 	// Make sure we have a script
// 	if( script_name == NULL ){
// 		syslog(SYSLOG_INFO,"serviced: %s: error: missing script name.", name);
// 		return -1;
// 	}

// 	// No valid action was found
// 	if( action == NULL )
// 	{
// 		syslog(SYSLOG_INFO,"serviced: %s: action '%s' not implemented.", name, action);
// 		return -1;
// 	}

// 	// Create the script path
// 	//snprintf(script_path, 512, "/etc/services/%s/%s", name, script_name);
// 	const char* argv[3] = { script_name->u.string.ptr, action_name, NULL };

// 	// Is this more than just a name?
// 	if( action->type == json_string ||
// 		(action->type == json_object && action->u.object.length == 1) )
// 	{
// 		// Just a name, so we just execute the action
// 		pid_t pid = fork();
// 		if( pid == 0 )
// 		{
// 			argv[1] = action_name;
// 			int result = execve(argv[0], argv, environ);
// 			if( result < 0 ){
// 				syslog(SYSLOG_INFO,"serviced: %s: failed to execute target: %d", argv[0], errno);
// 				exit(-1);
// 			}
// 		} else if( pid < 0 ){
// 			syslog(SYSLOG_INFO,"serviced: %s: failed to fork process: %d", name, errno);
// 			return -1;
// 		}
// 	} else {
// 		// Pre, and post action lists
// 		json_value* pre = json_object_get(action, "pre");
// 		json_value* post = json_object_get(action, "post");
// 		// Should we actually execute this named action?
// 		// Or just the pre/post?
// 		json_value* phony = json_object_get(action, "phony");

// 		// Execute pre actions if present
// 		if( pre != NULL && pre->type == json_array ){
// 			for(int i = 0; i < json_array_len(pre); i++){
// 				if( json_array_get(pre, i)->type != json_string ) continue;
// 				_service_call(name, manifest, json_array_get(pre, i)->u.string.ptr);
// 			}
// 		}
// 		// Execute actual action if not phony
// 		if( phony == NULL || (phony->type == json_boolean && !phony->u.boolean) )
// 		{
// 			pid_t pid = fork();
// 			if( pid == 0 )
// 			{
// 				argv[1] = action_name;
// 				int result = execve(argv[0], argv, environ);
// 				if( result < 0 ){
// 					syslog(SYSLOG_INFO,"serviced: %s: failed execute target: %d", argv[0], errno);
// 					exit(-1);
// 				}
// 			} else if( pid < 0 ){
// 				syslog(SYSLOG_INFO,"serviced: %s: failed to fork process: %d", name, pid);
// 				return -1;
// 			}
// 		}
// 		// Execute post action
// 		if( post != NULL && post->type == json_array ){
// 			for(int i = 0; i < json_array_len(post); i++){
// 				if( json_array_get(post, i)->type != json_string ) continue;
// 				_service_call(name, manifest, json_array_get(post, i)->u.string.ptr);
// 			}
// 		}
// 	}
// }


json_value* json_array_get(json_value* array, int idx)
{
	return array->u.array.values[idx];
}

int json_array_len(json_value* array)
{
	return array->u.array.length;
}

json_value* json_object_get(json_value* object, const char* name)
{
	for( json_object_entry* entry = object->u.object.values;
		entry <= &object->u.object.values[object->u.object.length-1];
		entry++ )
	{
		if( strncmp(entry->name, name, entry->name_length) == 0 ){
			return entry->value;
		}
	}
	return NULL;
}

// Read an entire file into memory at once
char* read_file(const char* name, long* pLength)
{
	// Length of the file
	long length = 0;
	// Open File
	FILE* file = fopen(name, "r");
	if( file == NULL ){
		return NULL;
	}
	// Find the length of the file
	// by seeking to the end
	fseek(file, 0, SEEK_END);
	length = ftell(file);
	fseek(file, 0, SEEK_SET);
	// Allocate a buffer to hold the file
	char* buffer = malloc(length);
	if( buffer == NULL ){
		fclose(file);
		return NULL;
	}
	// Read the file
	fread(buffer, length, 1, file);
	// Close and return
	fclose(file);
	// Save the length if supplied
	if( pLength ) *pLength = length;
	return buffer;
}

// Start all services for boot up
int services_start( void )
{
	DIR* dirp = opendir("/etc/services");
	struct dirent* dirent = NULL;
	char path[256];
	struct stat st;

	if( dirp == NULL ){
		syslog(SYSLOG_ERR, "serviced: unable to open service directory: %d", errno);
	}

	// Iterate over all directory entries
	while( dirent = readdir(dirp) ){

		// Ignore hidden files/folders
		if( dirent->d_name[0] == '.' ){
			continue;
		}

		// Get the directory stat information
		snprintf(path, 256, "/etc/services/%s", dirent->d_name);
		// DIR* dir = opendir(path);
		// if( dir == NULL ){
		// 	syslog(SYSLOG_ERR, "serviced: %s: error: unable to open directory: %d", path, errno);
		// } else {
		// 	closedir(dir);
		// }
		if( stat(path, &st) < 0 ){
			syslog(SYSLOG_INFO,"serviced: %s: error: unable to stat directory: %d", path, errno);
			continue;
		}

		// Make sure this entry is a directory
		if( !S_ISDIR(st.st_mode) ){
			continue;
		}

		// Start the service
		syslog(SYSLOG_INFO, "serviced: starting %s.", dirent->d_name);
		if( service_start(dirent->d_name) < 0 ){
			syslog(SYSLOG_WARN, "serviced: %s: failed to start.", dirent->d_name);
		}
	}

	closedir(dirp);
}

int services_stop( void )
{
	DIR* dirp = opendir("/etc/services");
	struct dirent* dirent = NULL;
	char path[256];
	struct stat st;

	// Iterate over all directory entries
	while( dirent = readdir(dirp) ){

		// Get the directory stat information
		snprintf(path, 256, "/etc/services/%s");
		if( stat(path, &st) < 0 ){
			syslog(SYSLOG_INFO,"serviced: %s: error: unable to stat directory.");
			continue;
		}

		// Make sure this entry is a directory
		if( !S_ISDIR(st.st_mode) ){
			continue;
		}

		// Start the service
		service_stop(dirent->d_name);
	}

	closedir(dirp);
}

// int service_call(const char* name, const char* action)
// {
// 	json_value* manifest = _service_open(name);
// 	if( manifest == NULL ){
// 		return -1;
// 	}

// 	int result = _service_call(name, manifest, action);

// 	json_value_free(manifest);

// 	return result;
// }