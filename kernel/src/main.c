#include "kernel.h"
#include "descriptor_tables.h"
#include "timer.h"
#include "paging.h"
#include "kmem.h"
#include "fs.h"
#include "task.h"
#include "multiboot.h"
#include <sys/fcntl.h>
#include "sys/mount.h"
#include "elf/elf32.h"
#include "error.h"
#include "sys/stat.h"
#include "sys/types.h"
#include <unistd.h>
#include "exec.h"
#include "pci.h"
#include "block.h"
#include <stdio.h>
#include "serial.h"
#include "ata.h"
//#include "ext2.h"
#include <dirent.h>
// testing spinlock
#include "spinlock.h"
#include "cmos.h"
#include "ext2fs/ext2.h"
#include "syslog.h"
#include "ps2.h"

int initfs_install(multiboot_info_t* mb);

int kmain( multiboot_info_t* mb );
int kmain( multiboot_info_t* mb )
{	
	int result = 0;
	mb = (multiboot_info_t*)( (u32)mb + KERNEL_VIRTUAL_BASE );
	//clear_screen();
	initialize_descriptor_tables();
	asm volatile("sti");
	init_timer(1000); // init the timer at 1000hz
	printk("Initializing paging... \n");
	init_paging(mb); // initialize the page tables and enable paging

	char cpu_vendor[16] = {0};
	u32 max_code = cpuid_string(CPUID_GETVENDORSTRING, cpu_vendor);
	
	printk("CPU Vendor String: %s (maximum supported cpuid code: %d)\n", &cpu_vendor[0], max_code);
	
	
	printk("Initializing virtual filesystem... \n");
	initialize_filesystem();
	
	printk("Enumerating PCI bus... \n");
	pci_enumerate();
	
	printk("Initializing multitasking subsystem... \n");
	task_init();
	
	printk("Initializing PS/2 Layer...\n");
	ps2_init();
	
	printk("Registering elf32 executable type... \n");
	elf_register();
	
	printk("Loading initfs tarball... \n");
	initfs_install(mb);
	
	printk("Loading IDE driver... \n");
	int error = ide_load();
	if( error != 0 ){
		printk("error: unable to load IDE driver!\n");
	}
	
	printk("Creating initfs disk nodes...\n");
	dev_t devid;
	for(int d = 0; d < 4; ++d)
	{
		for(int p = 0; p <= 4; ++p)
		{
			devid = makedev(0, (d | (p << 4)));
			if( get_block_device(devid) == NULL ) continue;
			char pathname[256] = {0};
			if( p != 0 ){
				sprintf(pathname, "/hd%c%d", 'a'+(char)d, p);
			} else {
				sprintf(pathname, "/hd%c", 'a'+(char)d);
			}
			error = sys_mknod(pathname, S_IFBLK, devid);
			if( error != 0 ){
				printk("%s: unable to create filesystem node: %d\n", pathname, error);
			}
			printk("created temporary disk node %s for device 0x%02X\n", pathname, devid);
		}
	}
	
	printk("Loading Ext2 Filesystem Support...\n");
	e2_install_filesystem();
	//error = sys_insmod("/ext2fs.o");
	if( error != 0 ){
		printk("Unable to load module ext2fs.o. error code %d\n", error);
	}
	//printk("Setting up Ext2 filesystem support... \n");
	//e2_setup();
	
	// Mount the Ext2 Filesystem from the root hard disk partition
	printk("Mounting /hda1 to /...\n");
	error = sys_mount("/hda1", "/", "ext2", 0/*MS_RDONLY*/, NULL);
	if( error != 0 ){
		printk("error: unable to mount device. error code %d\n", error);
	} else {
		printk("Root filesystem mounted successfully.\n");
	}
	
	// switch printk to serial in case something calls it for some reason...
	syslog(KERN_NOTIFY, "Switching printk to serial output just in case...");
	switch_printk_to_serial();
	
	syslog(KERN_NOTIFY, "Loading user-mode VGA TTY driver...");
	error = sys_insmod("/bin/vtty.o");
	if( error != 0 ){
		syslog(KERN_WARN, "Unable to load module. error code %d", error);
	}
	
	syslog(KERN_NOTIFY, "Creating virtual tty node /dev/vtty...");
	result = sys_mknod("/dev/vtty", S_IFCHR, makedev(0x02, 0));
	if( result != 0 && result != (-EEXIST) ){
		syslog(KERN_WARN, "Unable to create vtty node. error code %d", result);
	}
	
	syslog(KERN_NOTIFY, "Setting up system logging interface...");
	result = begin_syslog_daemon("/var/log/syslog");
	if( result < 0 ){
		printk("error: unable to initialize kernel system log.\n");
	}
	
	const char* INIT = "/bin/init";
	syslog(KERN_NOTIFY, "Loading INIT task (%s).", INIT);
	
	pid_t pid = sys_fork();
	if( pid < 0 ) {
		syslog(KERN_ERR, "Unable to fork process. error code %d", pid);
	} else if( pid == 0 ) {
		sys_open("/dev/vtty", O_RDWR, 0);
		sys_open("/dev/vtty", O_RDWR, 0);
		sys_open("/dev/vtty", O_RDWR, 0);
		char* argv[2] = {(char*)INIT, NULL}, *envp[2] = {(char*)"PATH=/bin", NULL};
		error = sys_execve(INIT, argv, envp);
		syslog(KERN_NOTIFY, "INIT: unable to execute %s. error code %d", INIT, error);
		syslog(KERN_NOTIFY, "INIT: killing forked init task...");
		sys_exit(-1);
	}
	
	enablei();
	while(1){
		asm volatile("hlt");
	}
	
	return (int)0xdeadbeaf;
}