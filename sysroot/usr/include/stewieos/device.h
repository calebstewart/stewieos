#ifndef _KERNEL_DEVICE_H_
#define _KERNEL_DEVICE_H_

#include "stewieos/kernel.h"

#define DEVICE_ID_ANY ((void*)0)
#define DEVICE_EVENT_ANY ((int)0)

typedef struct _device_type
{
	char name[64]; // name of the device type
	size_t length; // length of the device structure
	device_type_ident_t ident; // device type identifier
} device_type_t;

typedef struct _input_device_ops
{
	int(*attach)(input_device_t* device);
	int(*detach)(input_device_t* device);
	int(*send)(input_device_t* device, input_event_t* event);
	int(*poll)(input_device_t* device, input_event_t* event);
};
	

typedef struct _device
{
	device_type_t* type; // device type
	void* unid; // unique identifier (device specific)
} device_t;

int device_dispatch(device_t* source, input_event_t* event);
int device_listen(device_type_t* device_type, void* unid, int event_mask, device_handler_t* handler);
device_t* device_attach(device_type_t* type, void* unid);
void device_detach(device_t* device);

#endif
