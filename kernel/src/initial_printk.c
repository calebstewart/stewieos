#include "kernel.h"
#include "timer.h"
#include "serial.h"
#include "fs.h"
#include "error.h"
#include <sys/stat.h>
#include <fcntl.h>

#define CURSOR_INDEX(monitor) (( (monitor).cx + (monitor).cy*(monitor).width )*2)

#define P_LEFT		0x01		// Left justify
#define P_SIGN		0x02		// Always show the sign (show + symbol)
#define P_SPACE		0x04		// if no sign is to be shown, add a space
#define P_PAD0		0x08		// Pad with zero's instead of spaces
#define P_NOWIDTH	0x10		// Width specified in argument list, just before argument
#define P_NOPREC	0x20		// Precision specified in argument list
#define P_PREFIX	0x40		// Use 0,0x,and 0X prefixes
#define P_UPPER		0x80		// use uppercase for hex digits

#define STEWIEOS_TAB_WIDTH 5

/* struct: monitor_info
 * purpose:
 * 	encapsulates the state of the VGA textmode console
 */
struct monitor_info
{
	int cx, cy; // cursor x and y
	char color; // text color (foreground and background)
	char* screen; // pointer to the screen buffer
	int width, height; // width and height of the screen
};
 
// the actual monitor_info structure in memory
static struct monitor_info monitor = {
	0, 0,				// cursor initially at 0,0
	0x0F,				// Text color (initial: White on Black) 
	(char*)0xC00B8000,		// Pointer to the video memory
	80, 25				// width and height of the buffer
};
static char output_level_colors[3] = {
	0x0F,				// Default output level (0), White on black
	0x01,				// Output level 2, Blue on black
	0x04,				// Output level 3, Red on black
};

//int(*internal_printk)(const char*,__builtin_va_list) = initial_printk;

static void initial_vga_put_char(char c, int flags, int width, int precision);			// puts a character to the screen
static void serial_put_char(char c, int flags, int width, int precision);			// puts a character out to the serial port
static int put_str(const char* s, int flags, int width, int precision);				// put a string to the the screen
static int put_int(long int v, int flags, int width, int precision);				// put an integer on the screen
static int put_uint(unsigned long int v, int flags, int width, int precision, int base);	// put an unsigned integer on the screen
void clear_screen( void );									// clear the screen
static void update_cursor( void );								// synchronize the software cursor with the hardware (visual) one.
static void scroll_screen(int count);								// Scroll count lines upwards

void set_cursor_pos(unsigned int pos);

// The initial put character function simply outputs to the VGA text-mode console (later it will use the serial ports)
printk_putchar_func_t put_char = initial_vga_put_char;
printk_putstr_func_t custom_put_str = NULL;

void switch_printk_to_serial( void )
{
	put_char = serial_put_char;
}

int printk(const char* fmt, ...)
{
	__builtin_va_list ap;
	__builtin_va_start(ap, fmt);
	int result = internal_printk(fmt, ap);
	//update_cursor();
	return result;
}

/* function: put_int
 * params:
 * 	long int v - the value to write
 * 	int flags - the flags from printf
 * 	int width - width from printf
 * 	int precision - precision from printf
 */
static int put_int(long int v, int flags, int width, int precision)
{
	int count = 0;
	int digits = 0;
	long int shifter = (v < 0 ? -v : v);
	char buffer[33] = {0}; // long int isn't going to have an outrageously big number
	char* ptr = &buffer[31];
	buffer[32] = 0;
	if( (v >= 0 && ((flags & P_SIGN) || (flags & P_SPACE))) || v < 0 ){
		count++;
	}
	long int posv = v > 0 ? v : -v;
	
	do{
		digits++;
		shifter /= 10;
	} while( shifter );
	
	count += digits;
	
	if( !(flags & P_LEFT) ){
		while( digits < precision ){
			put_char('0', 0, 1, 0);
			count++; digits++;
		}
		while( count < width ){
			put_char((flags & P_PAD0) ? '0' : ' ', 0, 1, 0);
			count++;
		}
	}
	
	do{
		*--ptr = (char)('0' + (posv % 10));
		posv /= 10;
	}while(posv);
	
	if( v >= 0 && (flags & P_SIGN) ){
		put_char('+', 0, 0, 0);
	} else if( v >= 0 && flags & P_SPACE ){
		put_char(' ', 0, 0, 0);
	} else if( v < 0 ){
		put_char('-', 0, 0, 0);
	}
	
	put_str(ptr, 0, 0, 0);
	
	if( flags & P_LEFT ){
		while( count < width ){
			put_char((flags & P_PAD0) ? '0' : ' ', 0, 1, 0);
			count++;
		}
	}		
	
	return count;
}

static int put_str(const char* s, int flags, int width, int precision)
{
	int count = 0; 
	if( precision == 0 ) precision = 1000000; // This isn't right, but it's just an initial implementation...
	
	if( !(flags & P_LEFT) && width != 0)
	{
		int temp_prec = precision;
		const char* temp_s = s;
		while( temp_prec && *temp_s ){
			count++; temp_prec--; temp_s++;
		}
		while(count < width){
			put_char((flags & P_PAD0) ? '0' : ' ', 0, 0, 0);
			count++;
		}
		count = 0;
	}
	
	if( custom_put_str != NULL ){
		if( precision < (int)strlen(s) ){
			custom_put_str(s, precision);
			count += precision;
		} else {
			custom_put_str(s, strlen(s));
			count += strlen(s);
		}
	} else {
		while(*s && precision)
		{
			put_char(*s, 0, 0, 0);
			s++; precision--;
			count++;
		}
	}
	
	if( flags & P_LEFT && width != 0 ){
		while( count < width ){
			put_char((flags & P_PAD0)?'0' : ' ', 0, 0, 0);
			count++;
		}
	}
	
	return count;
}

static int put_uint(unsigned long int v, int flags, int width, int precision, int base)
{
	int count = 0;
	int digits = 0;
	unsigned long int shifter = v;
	char buffer[33] = {0}; // long int isn't going to have an outrageously big number
	char* ptr = &buffer[31];
	char letter_base = 'a';
	if( flags & P_SIGN ){
		count++;
	}
	
	memset(buffer, 0, 32);
	
	if( (flags & P_PREFIX) ){
		if( base == 8 ) count++;
		else if( base == 16 ) count+=2;
	}
	
	if( flags & P_UPPER ){
		letter_base = 'A';
	}
	
	do{
		digits++;
		shifter /= ((unsigned long int)base);
	} while( shifter );
	
	count += digits;
	
	if( flags & P_PREFIX ){
		put_char('0', 0, 0, 0);
		if( base == 16 ){
			put_char('x', 0, 0, 0);
		}
	}
	
	if( !(flags & P_LEFT) ){
		while( digits < precision ){
			put_char('0', 0, 0, 0);
			digits++; count++;
		}
		while( count < width ){
			put_char((flags & P_PAD0) ? '0' : ' ', 0, 1, 0);
			count++;
		}
	}
	
	do{
		if( (v%(unsigned long int)base) > 9 )
			*--ptr = (char)(letter_base + (char)((v%((unsigned long int)base))-10));
		else
			*--ptr = (char)('0' + (char)(v % ((unsigned long int)base)));
		v /= ((unsigned long int)base);
	}while(v);
	
	put_str(ptr, 0, 0, 0);
	
	if( flags & P_LEFT ){
		while( count < width ){
			put_char((flags & P_PAD0) ? '0' : ' ', 0, 1, 0);
			count++;
		}
	}		
	
	return count;
}

/* The initial printk function to print to the VGA console buffer */
int internal_printk(const char* format, __builtin_va_list ap)
{
	int count = 0;
	int flags = 0;
	char strWidth[5] = {0}; // shoud be enough to hold the width
	int width = 0;
	int precision = 0;
	char length = 0;
	char specifier = 0;
	char original_color = monitor.color;
	//__builtin_va_list ap;
	//__builtin_va_start(ap, format);
	
// 	put_char('[',0,0,0);
// 	put_uint(timer_get_ticks(), 0, 0, 0, 10);
// 	put_str("] ", 0, 0, 0);
	
	while(*format){
		
		if( *format == '%' )
		{
			flags = 0; width = 0; precision = 0; length = 0; specifier = 0;
			format++;
			// look for flags
			char flags_done = 0;
			while(!flags_done)
			{
				switch(*format)
				{
					case '-': flags |= P_LEFT; ++format; break;
					case '+': flags |= P_SIGN; ++format; break;
					case '0': flags |= P_PAD0; ++format; break;
					case ' ': flags |= P_SPACE; ++format; break;
					case '#': flags |= P_PREFIX; ++format; break;
					case 0: goto complete; break; 
					default: flags_done = 1;
				}
			}
			// look for width specifier
			if( *format == '*' ){
				flags |= P_NOWIDTH;
			} else if( *format != 0 ){
				if( *format >= '0' && *format <= '9' ){
					int i = 0, power = 1;
					while(*format >= '0' && *format <= '9' && i < 5){
						strWidth[4-i] = *format;
						format++; i++;
					}
					if( *format == 0 ) continue;
					for(i = i-1; i >= 0; i--){
						width += (strWidth[4-i]-'0')*power;
						power *= 10;
					}
				}
			} else continue;
			// look for precision specifier
			if( *format == '.' )
			{
				format++;
				if(*format == '*')
				{
					flags |= P_NOPREC;
				} else if( *format != 0 ) {
					if( *format >= '0' && *format <= '9' ){
						int i = 0, power = 1;
						char number[12] = {0};
						while(*format >= '0' && *format <= '9' && i < 12){
							number[11-i] = *format;
							format++; i++;
						}
						if( *format == 0 ) continue;
						for(i = i-1; i >= 0; i--){
							precision += (number[11-i]-'0')*power;
							power *= 10;
						}
					}
				} else continue;
			}
			
			if( *format == 'l' || *format == 'L' || *format == 'h' )
			{
				length = *format;
				format++;
				if(*format == 0) continue;
			}
			
			specifier = *format;
			format++;
			
			if( flags & P_NOWIDTH ){
				width = __builtin_va_arg(ap, int);
			}
			if( flags & P_NOPREC ){
				precision = __builtin_va_arg(ap, int);
			}
			
			if( specifier == 'd' || specifier == 'i' )
			{
				switch(length)
				{
					case 0: count += put_int(__builtin_va_arg(ap, int), flags, width, precision); break;
					case 'l': count += put_int(__builtin_va_arg(ap, long int), flags, width, precision); break;
					case 'h': count += put_int(__builtin_va_arg(ap, int), flags, width, precision); break;
				}
			} else if( specifier == 's' ) // end if specifier == integer
			{
				if( length == 0 ){
					count += put_str(__builtin_va_arg(ap, char*), flags, width, precision);
				}
			} else if( specifier == 'p' || specifier == 'u' || specifier == 'o' || specifier == 'x' || specifier == 'X' ) // end if specifier == string
			{
				int base = 10;
				if( specifier == 'o' ){
					base = 8;
				}else if( specifier == 'p' || specifier == 'x' || specifier == 'X' ) {
					base = 16;
					if( specifier == 'X' ) flags |= P_UPPER;
					if( specifier == 'p' ) flags |= P_PREFIX;
				}
				switch(length)
				{
					case 0: count += put_uint(__builtin_va_arg(ap, unsigned int), flags, width, precision, base); break;
					case 'h': count += put_uint(__builtin_va_arg(ap, unsigned int), flags, width, precision, base); break;
					case 'l': count += put_uint(__builtin_va_arg(ap, unsigned long int), flags, width, precision, base); break;
				}
			} else if( specifier == 'c' ){ // end if specifier == unsigned integer
				if( length == 0 ){
					put_char((char)__builtin_va_arg(ap, int), flags, width, precision);
					count++;
				}
			} else if( specifier == 'f' || specifier == 'F' || specifier == 'E' || specifier == 'g' ||
				specifier == 'G' || specifier == 'a' || specifier == 'A' ) { // end if specifier == character
				if( length == 0 ){
					//count += put_double(__builtin_va_arg(ap, double), flags, width, precision);
				} else if( length == 'L' ){
					//count += put_double(__builtin_va_arg(ap, long double), flags, width, precision);
				}
			} else if( specifier == 'V' ){ // end if specifier == double
				// Change the output level
				if( width < 0 || width >= 3 ){ // output level out of range, default to 0
					width = 0;
				}
				monitor.color = output_level_colors[width];
			} else if( specifier == 'C' ){ // Specifier is to change the color
				monitor.color = (char)((width&0x0F) | 0x00);
			} else if( specifier == 't' ){ // dump kernel time
				put_char('[',0,0,0);
				put_uint(timer_get_ticks(), 0,0,0, 10);
				put_str("] ",0,0,0);
			}
			
			
		} else { // end format == '%'
			put_char(*format, 0, 0, 0);
			if( *format == '\n' ){
				put_char('[',0,0,0);
				put_uint(timer_get_ticks(), 0,0,0, 10);
				put_str("] ",0,0,0);
			}
			format++;
			count++;
		} // end format != '%'
	}
complete:
	// Reset the text color, and update the hardware cursor
	monitor.color = original_color;
	//update_cursor();
	return count;
}

static void serial_put_char(char c, int flags __attribute__((unused)), int width __attribute__((unused)), int precision __attribute__((unused)))
{
	serial_device_t* device = serial_get_device(0);
	serial_write(device, &c, 1);
}

/* function: put_char
 * params:
 * 	char c - the character to be written
 * purpose:
 * 	writes a single character to he screen at the current cursor
 * 	position
 */
static void initial_vga_put_char(char c, int flags, int width, int precision)
{
	UNUSED(precision);
	UNUSED(flags);
	UNUSED(width);
	// \r escape character (return to column 0)
	if( c == '\r' )
	{
		monitor.cx = 0;
	}
	
	// \n escape character (new line)
	if( c == '\n' )
	{
		monitor.cy++;
		monitor.cx = 0;
		if( monitor.cy == monitor.height ){
			monitor.cy--;
			scroll_screen(1);
		} // end monitor.cy == monitor.height
	} // end c == '\n'
	
	// \b escape character (backspace)
	if( c == '\b' ){
		if( monitor.cx == 0 ){
			if( monitor.cy != 0 ){
				monitor.cx = monitor.width-1;
				monitor.cy--;
			}
		} else { // end if monitor.cx == 0
			monitor.cx--;
			monitor.screen[CURSOR_INDEX(monitor)] = 0;
			monitor.screen[CURSOR_INDEX(monitor)+1] = monitor.color;
		} // end if monitor.cx != 0
	} // end if c == '\b'
	
	// \t escape character (tab)
	if( c == '\t' )
	{
		if( (monitor.width-monitor.cx) < STEWIEOS_TAB_WIDTH ){
			monitor.cx = monitor.width;
		} else {
			monitor.cx += STEWIEOS_TAB_WIDTH;
		}
	}
	
	// printable character
	if( c >= ' ' ){
		monitor.screen[CURSOR_INDEX(monitor)] = c;
		monitor.screen[CURSOR_INDEX(monitor)+1] = monitor.color;
		monitor.cx++;
	} // end c >= ' '
	
	// should we wrap the text?
	if( monitor.cx == monitor.width ){
		monitor.cx = 0;
		monitor.cy++;
		if( monitor.cy == monitor.height ){
			monitor.cy--;
			scroll_screen(1);
		}
	} // end monitor.cx == monitor.width
	
	update_cursor();
	
} // end put_char

/* function: cursor_pos
 * params:
 * 	none
 * purpose:
 * 	Retrieve the current cursor as (x + y*width)
 */
// unsigned int get_cursor_pos(void )
// {
// 	return monitor.cx + monitor.cy*monitor.width;
// }

/* function: set_cursor_pos
 * params:
 * 	idx: the cursor position as (x+y*width)
 * purpose:
 * 	set the cursor position
 * note:
 * 	Does not update the hardware cursor.
 */
void set_cursor_pos(unsigned int idx)
{
	monitor.cx = idx % monitor.width;
	monitor.cy = (unsigned int)(idx / monitor.width);
}

/* function: clear_screen
 * params:
 * 	none
 * purpose:
 * 	clears the entire screen to the current attributes
 */
void clear_screen( void )
{
	for(int x = 0; x < monitor.width; ++x){
		for(int y = 0; y < monitor.height; ++y){
			monitor.screen[0 + monitor.cx + monitor.cy*monitor.width] = 0; // character
			monitor.screen[1 + monitor.cx + monitor.cy*monitor.width] = monitor.color; // and attribute
		}
	}
} // end clear_screen

/* function: scroll_screen
 * params:
 * 	count -- the number of lines to scroll
 * purpose:
 * 	scroll the page upwards by count lines
 */
static void scroll_screen(int count)
{
	for(int ln = 0; ln < monitor.height-count; ++ln)
	{
		memcpy(&monitor.screen[ln*monitor.width*2], &monitor.screen[(ln+1)*monitor.width*2], (size_t)(monitor.width*2));
	}
 	for(int ln = monitor.height-count; ln < monitor.height; ++ln)
 	{
		for(int i = 0; i < monitor.width; ++i){
			monitor.screen[(i+(ln*monitor.width))*2] = 0;
			monitor.screen[1+(i+(ln*monitor.width))*2] = 0;
		}
 		//memset(&monitor.screen[ln*monitor.width*2], 0, (size_t)(monitor.width*2));
 	}
}

/* function: update_cursor
 * params:
 * 	none
 * purpose:
 * 	updates the hardware (visible) cursor to the current software position
 */
static void update_cursor( void )
{
	unsigned short idx = (unsigned short)(monitor.cy * monitor.width + monitor.cx); // grab the index ofthe cursor position
	outb(0x3D4, 0x0E);
	outb(0x3D5, (unsigned char)(idx >> 8)); // send the lower 8 bits
	outb(0x3D4, 0x0F);
	outb(0x3D5, (unsigned char)(idx)); // and the higher 8 bits
} // end update_cursor