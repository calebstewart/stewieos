#include "exec.h"

int test_load(module_t* module);
int test_remove(module_t* module);

module_t module_info __attribute__((section(".module_info"))) = {
	.m_name = "test_mod",
	.m_load = test_load,
	.m_remove = test_remove,
};

int test_load(module_t* module)
{
	return 42;
}

int test_remove(module_t* module)
{
	return 42*42;
}