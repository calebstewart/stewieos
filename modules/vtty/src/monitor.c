#include "monitor.h"

void vga_get_cursor(int* cx, int* cy);
void vga_sync_cursor( void );
void monitor_newline( void );
void monitor_carriagereturn( void );
void monitor_tab( void );
void monitor_backspace( void );
void monitor_advance( void );
void monitor_retreat( void );
void monitor_scroll( void );

typedef struct _vga_textmode
{
	char attr; // the attribute byte for writing
	int cx, cy; // the cursor position (cx is colum, cy is row)
	char* data; // the data buffer for the screen
	int width, height; // the width and height of the buffer
} vga_textmode_t;

static vga_textmode_t vga = {
	.attr = 0x0F,			// Initial text color of white on black
	.cx = 0, .cy = 0,		// Cursor position 0,0 (top-left)
	.data = (char*)0xC00B8000,	// Pointer to the VGA data
	.width = 80, .height = 25	// width and height of vga text-mode console
};

void vga_sync_cursor( void )
{
	unsigned short idx = (unsigned short)( vga.cx + vga.cy*vga.width );
	outb(0x3D4, 0x0E);
	outb(0x3D5, (unsigned char)(idx >> 8));
	outb(0x3D4, 0x0F);
	outb(0x3D5, (unsigned char)(idx));
}

void monitor_scroll( void )
{
	// shift rows up
	for(int r = 0; r < (vga.height-1); ++r)
	{
		memcpy(vga.data + (r*vga.width*2), vga.data + (r+1)*vga.width*2, vga.width*2);
	}
	// clear last row
	for(int c = 0; c < vga.width; ++c){
		*(vga.data+(c+(vga.height-1)*vga.width)*2) = ' ';
		*(vga.data+(c+(vga.height-1)*vga.width)*2 + 1) = vga.attr;
	}
}

void monitor_newline( void )
{
	vga.cy++;
	if( vga.cy == vga.height )
	{
		vga.cy--;
		monitor_scroll();
	}
	vga.cx = 0;
}

void monitor_carriagereturn( void )
{
	vga.cx = 0;
}

void monitor_tab( void )
{
	for(int i = 0; i < 5; ++i){
		monitor_putchar(' ');
	}
}

void monitor_backspace( void )
{
	monitor_retreat();
	*(vga.data + (vga.cx + vga.cy*vga.width)*2) = ' ';
	*(vga.data + (vga.cx + vga.cy*vga.width)*2 + 1) = vga.attr;
}

void monitor_advance( void )
{
	vga.cx++;
	if( vga.cx >= vga.width )
	{
		monitor_newline();
	}
}

void monitor_retreat( void )
{
	vga.cx--;
	if( vga.cx < 0 )
	{
		if( vga.cy == 0 ){
			vga.cx = 0;
			return;
		}
		vga.cy--;
		vga.cx = vga.width-1;
	}
}

int monitor_init(module_t* module __attribute__((unused)))
{
	monitor_clear();
	monitor_moveto(0, 0);
	monitor_setattr(0x0F);
	return 0;
}

int monitor_quit(module_t* module __attribute__((unused)))
{
	return 0;
}

int monitor_clear( void )
{
	for(int x = 0; x < vga.width; ++x){
		for(int y = 0; y < vga.height; ++y){
			*(vga.data + (x + y*vga.width)*2) = ' ';
			*(vga.data + (x + y*vga.width)*2 + 1) = vga.attr;
		}
	}
	return 0;
}

int monitor_moveto(int cx, int cy)
{
	vga.cx = cx;
	vga.cy = cy;
	vga_sync_cursor();
	return 0;
}

int monitor_setattr(unsigned char attr)
{
	vga.attr = (char)attr;
	return 0;
}

int monitor_putchar(char c)
{
	switch(c)
	{
		case '\n': monitor_newline(); break;
		case '\r': monitor_carriagereturn(); break;
		case '\t': monitor_tab(); break;
		case '\b': monitor_backspace(); break;
		default:
			if( c >= 32 && c < 0x7f ){
				*(vga.data + (vga.cx + vga.cy*vga.width)*2) = c;
				*(vga.data + (vga.cx + vga.cy*vga.width)*2 + 1) = vga.attr;
				monitor_advance();
			}
	}
	vga_sync_cursor();
	return 0;
}
