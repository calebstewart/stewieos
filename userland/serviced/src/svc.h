#ifndef _SVC_H_
#define _SVC_H_

#include <unistd.h>
#include <json-parser/json.h>

// Start all boot-time services
int services_start();
// Stop all services before shutdown
int services_stop();

// Start the service
int service_start(const char* name);
// Stop the service
int service_stop(const char* name);
// Restart the service
int service_restart(const char* name);
// Call a service action
int service_call(const char* name, const char* action);
// Check if the service is running
int service_running(const char* name);
// Open the service manifest
json_value* service_open(const char* name);

#endif