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

// testing spinlock
#include "spinlock.h"

tick_t my_timer_callback(tick_t, struct regs*);
tick_t my_timer_callback(tick_t time, struct regs* regs)
{
	UNUSED(regs);
	printk("timer_callback: time: %d+%d/1000\n", (time / 1000), time%1000);
	return time+timer_get_freq();
}

// defined in initial_printk.c
//void clear_screen(void);

int global_var = 0;

void multitasking_entry( multiboot_info_t* mb);
int initfs_install(multiboot_info_t* mb);

int kmain( multiboot_info_t* mb );
int kmain( multiboot_info_t* mb )
{	
	//clear_screen();
	initialize_descriptor_tables();
	asm volatile("sti");
	init_timer(1000); // init the timer at 1000hz
	printk("initializing paging... ");
	unsigned int curpos = get_cursor_pos();
	printk("\n");
	init_paging(mb); // initialize the page tables and enable paging
	printk_at(curpos, " done.\n");

	char cpu_vendor[16] = {0};
	u32 max_code = cpuid_string(CPUID_GETVENDORSTRING, cpu_vendor);
	
	printk("CPU Vendor String: %s (maximum supported cpuid code: %d)\n", &cpu_vendor[0], max_code);
	
	
	/*printk("Registering timer callback for the next second... ");
	int result = timer_callback(timer_get_ticks()+timer_get_freq(), my_timer_callback);
	printk("done (result=%d)\n", result);*/
	
	printk("Initializing virtual filesystem... ");
	initialize_filesystem();
	printk("done.\n");
	
	pci_enumerate();
	
	printk("Initializing multitasking subsystem...\n");
	task_init();
	
	printk("init: forking init task... ");
	
	pid_t pid = sys_fork();
	
	if( pid == 0 ){
		multitasking_entry(mb);
		// this should never happen
		sys_exit(-1);
	}
	
	while(1);
	
	return (int)0xdeadbeaf;
}

#include "ata.h"
#include "ext2.h"
#include <dirent.h>

void multitasking_entry( multiboot_info_t* mb )
{
	printk("done.\n");
	
	initfs_install(mb);
	
	printk("INIT: loading module 'ide.o'\n");
	//int error = sys_insmod("/ide.o");
	int error = ide_load();
	if( error != 0 ){
		printk("INIT: unable to load module: %d\n", error);
	} else {
		printk("INIT: ide.o successfully loaded.\n");
	}
	
	printk("INIT: creating disk nodes...\n");
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
				printk("INIT: %s: unable to create filesystem node: %d\n", pathname, error);
			}
			printk("INIT: created disk node %s for device 0x%02X\n", pathname, devid);
		}
	}
	
// 	printk("INIT: opening /hda1 for reading...\n");
// 	int fd = sys_open("/hda1", O_RDONLY, 0);
// 	
// 	if( fd < 0 ){
// 		printk("INIT: error: unable to open /hda1! error code %d.\n", fd);
// 		return;
// 	}
// 	
// 	printk("INIT: reading superblock from /hda1...\n");
// 	char superblock[1024];
// 	sys_lseek(fd, 1024, SEEK_SET);
// 	sys_read(fd, superblock, 1024);
// 	
// 	printk("INIT: EXT2 Magic Bytes: 0x%04X\n", *((u16*)(&superblock[56])));
// 	
// 	sys_close(fd);

	e2_setup();
	
	printk("INIT: mounting /hda1 to /...\n");
	error = sys_mount("/hda1", "/", "ext2", MS_RDONLY, NULL);
	if( error != 0 ){
		printk("%2VINIT: error: unable to mount device. error code %d\n");
		return;
	}
	
	printk("INIT: successfully mounted /hda1 to /!\n");
	
	printk("INIT: attempting to open /...\n");
	int fd = sys_open("/", O_RDONLY, 0);
	if( fd < 0 ){
		printk("INIT: unable to open /. error code %d\n", fd);
	} else {
		printk("INIT: opened / successfully!\n%-6s\tFile Name\n", "Inode");
		struct dirent dirent;
		while( (error = sys_readdir(fd, &dirent, 1)) == 1 )
		{
			if( dirent.d_type == DT_BLOCK || dirent.d_type == DT_CHARACTER ){
				printk("%-6d\t%14C%s\n", dirent.d_ino, dirent.d_name);
			} else if( dirent.d_type == DT_DIRECTORY ){
				printk("%-6d\t%9C%s\n", dirent.d_ino, dirent.d_name);
			} else {
				printk("%-6d\t%15C%s\n", dirent.d_ino, dirent.d_name);
			}
		}
		if( error != 0 ){
			printk("INIT: unable to read directory. error code %d\n", error);
		}
		sys_close(fd);
	}
	
	fd = sys_open("/node", O_RDONLY, 0);
	sys_close(fd);
	
	printk("INIT: Finished.\n");
	
	return;
	
}