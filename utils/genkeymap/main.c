#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

enum _keycodes
{
	KEY_UNKNOWN, KEY_ESC, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0,
	KEY_MINUS, KEY_EQUAL, KEY_BACKSPACE, KEY_TAB, KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T,
	KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P, KEY_LEFTBRACKET, KEY_RIGHTBRACKET, KEY_ENTER,
	KEY_LEFTCTRL, KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L,
	KEY_SEMICOLON, KEY_SINGLEQUOTE, KEY_BACKTICK, KEY_LEFTSHIFT, KEY_BACKSLASH,
	KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M, KEY_COMMA, KEY_PERIOD, KEY_SLASH,
	KEY_RIGHTSHIFT, KEY_ASTERISK, KEY_LEFTALT, KEY_SPACE, KEY_CAPSLOCK, KEY_F1,
	KEY_F2, KEY_F3, KEY_F4,	KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_NUMLOCK,
	KEY_SCRLOCK, KEY_K7, KEY_K8, KEY_K9, KEY_KMINUS, KEY_K4, KEY_K5, KEY_K6, KEY_KPLUS,
	KEY_K1, KEY_K2, KEY_K3, KEY_K0, KEY_KPERIOD, KEY_F11, KEY_F12, KEY_KENTER, KEY_RIGHTCTRL,
	KEY_RIGHTALT, KEY_HOME, KEY_CURSORUP, KEY_PAGEUP, KEY_CURSORLEFT, KEY_CURSORRIGHT,
	KEY_END, KEY_CURSORDOWN, KEY_PAGEDOWN, KEY_INSERT, KEY_DELETE, KEY_LEFTGUI,
	KEY_RIGHTGUI, KEY_APPS, KEY_PRINTSCREEN, KEY_PAUSE, KEY_COUNT,
};

const char* keynamemap[KEY_COUNT] = {
	"unknown", "esc", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
	"-", "=", "backspace", "tab", "q", "w", "e", "r", "t",
	"y", "u", "i", "o", "p", "{", "}", "enter",
	"leftctrl", "a", "s", "d", "f", "g", "h", "j", "k", "l",
	";", "'", "`", "leftshift", "\\",
	"z", "x", "c", "v", "b", "n", "m", ",", ".", "/",
	"rightshift", "*", "leftalt", "space", "capslock", "f1",
	"f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10", "numlock",
	"scrolllock", "keypad7", "keypad8", "keypad9", "keypad_minus", "keypad4", "keypad5", "keypad6", "keypad_plus",
	"keypad1", "keypad2", "keypad3", "keypad0", "keypad_period", "f11", "f12", "keypad_enter", "rightctrl",
	"rightalt", "home", "up", "pageup", "left", "right", "end", "down", "pagedown", "insert", "delete", "leftgui",
	"rightgui", "apps", "printscreen", "pause"
};

unsigned char keymap[256];

static int get_vkey(const char* keyname)
{
	for(int i = 0; i < KEY_COUNT; ++i){
		if( strcasecmp(keyname, keynamemap[i]) == 0 ){
			return i;
		}
	}
	return -1;
}

int main(int argc, char** argv)
{
	
	
	if( argc != 2 ){
		fprintf(stderr, "Usage: %s keymap_name\n", argv[0]);
		return -1;
	}
	
	FILE* keymap = fopen(argv[1], "r");
	
	char keyname[64];
	char keyvalue[64];
	char keyopt[64];
	char* lineptr = NULL;
	size_t linememsz = 0;
	ssize_t linelen = 0;
	size_t lineno = 1;
	size_t parmcnt = 0;
	
	while( (linelen = getline(&lineptr, &linememsz, keymap)) != -1 )
	{
		if( lineptr[0] == '#' ) continue; // ignore comments
		parmcnt = sscanf(lineptr, "%s %s %s", keyname, keyvalue, keyopt);
		
		if( parmcnt < 2 ){
			fprintf(stderr, "error: line %d: format error. expected atleast two parameters.\n", lineno);
			return -1;
		}
		
		int vkey = get_vkey(keyname);
		if( vkey == -1 ){
			fprintf(stderr, "error: line %d: unknown key name '%s'.\n", lineno, keyname);
			return -1;
		}
		
		long int scancode = 0;
		if( strncasecmp("0x", keyvalue, 2) == 0 ){
			scancode = strtol(&keyvalue[2], NULL, 16)
		} else {
			scancode = strtol(keyvalue, NULL, 10);
		}
		
		if( scancode >= 128 || scancoe < 0 ){
			fprintf(stderr, "error: line %d: scancode value out of range. scancodes must be in the range [1..127].\n", lineno);
			return -1;
		}
		
		if( strcasecmp(keyopt, "shift") == 0 ){
			scancode += 128;
		} else if( strcasecmp(keyopt, "addupper") ){
			
		
	
	
}