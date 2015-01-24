#include <kernel.h>
#include <ps2.h>
#include <timer.h>
#include <linkedlist.h>
#include <descriptor_tables.h>
#include <kmem.h>

u8 ps2_port[2] = {1, 1};
tick_t ps2_poll_timeout = 100;
list_t ps2_listenlist[2] = {
	LIST_INIT(ps2_listenlist[0]),
	LIST_INIT(ps2_listenlist[1])
};

static void ps2_port1_interrupt(struct regs* regs ATTR((unused)))
{
	u8 data = ps2_read_data( );
	list_t* iter;
	
	list_for_each(iter, &ps2_listenlist[0]){
		ps2_listener_t* listener = list_entry(iter, ps2_listener_t, entry);
		listener->listener(PS2_PORT1, data);
	}
	
}

static void ps2_port2_interrupt(struct regs* regs ATTR((unused)))
{
	u8 data = ps2_read_data( );
	list_t* iter;
	
	list_for_each(iter, &ps2_listenlist[1]){
		ps2_listener_t* listener = list_entry(iter, ps2_listener_t, entry);
		listener->listener(PS2_PORT2, data);
	}
	
}

void ps2_init( void )
{
	u8 dual = 1;
	
	// Disable the ports and flush any pending data
	ps2_disable(PS2_PORT1 | PS2_PORT2); // disable devices
	ps2_read_data(); // discard any data in the output buffer
	
	// Setup the configuration parameters as we need (disable interrupts and translation)
	// also store the new configuration to check dual ports info
	u8 cfg = ps2_config(0, PS2_CFG_INT1 | PS2_CFG_INT2 | PS2_CFG_TRN2);
	
	// if the disable command didn't disable the 2nd port, then it doesn't exist
	dual = (cfg & PS2_CFG_DIS2) ? 1 : 0;
	
	// Perform the controller self test
	u8 result = ps2_self_test(PS2_TEST_CTRL);
	if( result != PS2_RESULT_PASS ){
		syslog(KERN_ERR, "PS/2 Controller Self-Test failed with result %d!", result);
		return;
	}
	
	// If dual channel is still a possibility, test for the second port
	// by enabling it and then checking the configuration
	if( dual )
	{
		ps2_enable(PS2_PORT2);
		if( (ps2_config(0,0) & PS2_CFG_DIS2) ){
			dual = 0;
		} else {
			ps2_disable(PS2_PORT2);
		}
	}
	
	// Perform self-test on port 1
	result = ps2_self_test(PS2_TEST_PORT1);
	if( result != PS2_RESULT_PASS ){
		syslog(KERN_NOTIFY, "PS/2 Port 1 Self-Test failed with result %d!", result);
		ps2_port[PS2_PORT1] = 0;
	}
	
	// Perform self test on port two if needed
	if( dual )
	{
		result = ps2_self_test(PS2_TEST_PORT2);
		if( result != PS2_RESULT_PASS ){
			syslog(KERN_NOTIFY, "PS/2 Port 2 Self-Test failed with result %d!", result);
			ps2_port[PS2_PORT2] = 0;
			dual = 0;
		}
	} else {
		ps2_port[PS2_PORT2] = 0;
	}
	
	register_interrupt(IRQ1, ps2_port1_interrupt);
	register_interrupt(IRQ12, ps2_port2_interrupt);
	
	// Enable the respective ports
// 	if( ps2_port[PS2_PORT1] ){
// 		ps2_enable(PS2_PORT1);
// 	}
// 	if( ps2_port[PS2_PORT2] ){
// 		ps2_enable(PS2_PORT2);
// 	}
	
	
}

u8 ps2_config(u8 ena, u8 dis)
{
	// Only these bits are able to be changed
	ena &= 0x73; dis &= 0x73;
	
	// Read in the configuration byte
	ps2_send_cmd(PS2_CMD_RDCFG);
	u8 cfg = ps2_read_data( );
	
	// If we aren't changing it, just return it
	if( ena == 0 && dis == 0 ) return cfg;
	
	// Make the modifications
	cfg = (u8)(cfg & ~dis); // disable the given features
	cfg = (u8)(cfg | ena); // enable the given features
	
	// write the configuration back to the controller
	ps2_send_cmd(PS2_CMD_WRCFG);
	ps2_send_data(PS2_CTRL, cfg);
	
	// Return the new configuation byte
	return cfg;
}

u8 ps2_poll(u8 buffer)
{
	// if we didn't specify a buffer, return
	if( (buffer & (PS2_INPUT_BUFFER | PS2_OUTPUT_BUFFER)) == 0 ) return 0;
	// Get the polling start time
	tick_t start = timer_get_ticks();
	// loop until the buffers are empty
	while( (ps2_status() & buffer) != buffer && timer_get_ticks() < (start+ps2_poll_timeout) );
	if( (ps2_status() & buffer) != buffer ){
		return PS2_RESULT_TIMEOUT;
	}
	return 0;
}

void ps2_send_cmd(u8 cmd)
{
	if( ps2_poll(PS2_INPUT_BUFFER) == PS2_RESULT_TIMEOUT ){
		return;
	}
	outb(PS2_CMD, cmd);
}

void ps2_send_data(u8 port, u8 data)
{
	if( port == PS2_PORT2 ){
		ps2_send_cmd(PS2_CMD_WRINP2); // write to the second ps2 port if requested
	}
	if( ps2_poll(PS2_INPUT_BUFFER) == PS2_RESULT_TIMEOUT ){
		return;
	}
	outb(PS2_DATA, data);
}

u8 ps2_read_data( void )
{
	if( ps2_poll(PS2_OUTPUT_BUFFER) == PS2_RESULT_TIMEOUT ){
		return 0;
	}
	return inb(PS2_DATA);
}

void ps2_enable( u8 ports )
{
	if( ports & PS2_PORT1 ){
		ps2_send_cmd(PS2_CMD_ENA1);
		ps2_config(PS2_CFG_INT1, 0);
	}
	if( ports & PS2_PORT2 ){
		ps2_send_cmd(PS2_CMD_ENA2);
		ps2_config(PS2_CFG_INT2, 0);
	}
}

void ps2_disable( u8 ports )
{
	if( ports & PS2_PORT1 ){
		ps2_send_cmd(PS2_CMD_DIS1);
	}
	if( ports & PS2_PORT2 ){
		ps2_send_cmd(PS2_CMD_DIS2);
	}
}

u8 ps2_self_test( u8 tests )
{
	u8 result = 0;
	if( tests & PS2_TEST_CTRL )
	{
		ps2_send_cmd(PS2_CMD_TEST);
		result = ps2_read_data( );
		if( result == 0xFC ){
			return PS2_RESULT_CTRL;
		}
	}
	if( tests & PS2_TEST_PORT1 )
	{
		ps2_send_cmd(PS2_CMD_TES1);
		result = ps2_read_data();
		if( result != PS2_RESULT_PASS ){
			return result;
		}
	}
	if( tests & PS2_TEST_PORT2 )
	{
		ps2_send_cmd(PS2_CMD_TES2);
		result = ps2_read_data();
		if( result != PS2_RESULT_PASS ){
			return (u8)(result + 0x10); // second port error is offset by 16
		}
	}
	
	return PS2_RESULT_PASS;
}

u8 ps2_reset(u8 port)
{
	if( port != PS2_PORT1 && port != PS2_PORT2 ){
		return PS2_RESULT_PASS;
	}
	ps2_send_data(port, PS2_DEVCMD_RESET);
	if( ps2_poll(PS2_OUTPUT_BUFFER) == PS2_RESULT_TIMEOUT ){
		return PS2_RESULT_TIMEOUT;
	}
	u8 data = ps2_read_data( );
	
	return (data == 0xFA ? PS2_RESULT_PASS : PS2_RESULT_FAIL);
}

ps2_listener_t* ps2_listen(u8 port, ps2_device_listener_t func)
{
	if( port > 2 || port < 1 ) return NULL;
	ps2_listener_t* listener = (ps2_listener_t*)kmalloc(sizeof(ps2_listener_t));
	if( !listener ) {
		return NULL;
	}
	
	listener->listener = func;
	INIT_LIST(&listener->entry);
	
	list_add(&listener->entry, &ps2_listenlist[port-1]);
	
	return listener;
}

void ps2_unlisten(ps2_listener_t* listener)
{
	if( !listener ) return;
	
	list_rem(&listener->entry);
	kfree(listener);
}