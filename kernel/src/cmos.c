#include "cmos.h"
#include "error.h"

static int g_nmi_state = 1;
unsigned int mon_to_days[13] = {
	0,
	31,
	31+28,
	31+28+31,
	31+28+31+30,
	31+28+31+30+31,
	31+28+31+30+31+30,
	31+28+31+30+31+30+31,
	31+28+31+30+31+30+31+31,
	31+28+31+30+31+30+31+31+30,
	31+28+31+30+31+30+31+31+30+31,
	31+28+31+30+31+30+31+31+30+31+30,
	31+28+31+30+31+30+31+31+30+31+30+31
};

int cmos_nmi_set(int state)
{
	int tmp = g_nmi_state;
	g_nmi_state = (state > 0);
	outb(CMOS_PORT_REG, (u8)(g_nmi_state << 7));
	return tmp;
}

u8 cmos_read(int reg)
{
	// select the register, don't allow this function to modify NMI state
	outb(CMOS_PORT_REG, (u8)((g_nmi_state<<7) | (reg & 0x7f)));
	return inb(CMOS_PORT_DATA);
}

void cmos_write(int reg, u8 v)
{
	outb(CMOS_PORT_REG, (u8)((g_nmi_state<<7) | (reg & 0x7f)));
	outb(CMOS_PORT_DATA, v);
}

time_t rtc_read(void)
{
	// Read all parameters
	u8 s = cmos_read(CMOS_RTC_SEC);
	u8 m = cmos_read(CMOS_RTC_MIN);
	u8 h = cmos_read(CMOS_RTC_HRS);
	u8 d = cmos_read(CMOS_RTC_DAY);
	u8 mo = cmos_read(CMOS_RTC_MON);
	int y = cmos_read(CMOS_RTC_YRS); // we assume the RTC reports a year in the 2000s, since I wrote this in 2014...
	
	u8 last_s, last_m, last_h, last_d, last_mo;
	int last_y;
	
	do
	{
		last_s = s;
		last_m = m;
		last_h = h;
		last_d = d;
		last_mo = mo;
		last_y = y;
		
		s = cmos_read(CMOS_RTC_SEC);
		m = cmos_read(CMOS_RTC_MIN);
		h = cmos_read(CMOS_RTC_HRS);
		d = cmos_read(CMOS_RTC_DAY);
		mo = cmos_read(CMOS_RTC_MON);
		y = cmos_read(CMOS_RTC_YRS); // we assume the RTC reports a year in the 2000s, since I wrote this in 2014...
		
	}while( last_s != s || last_m != m || last_h != h || last_d != d || last_mo != mo || last_y != y );
	
	u8 stb = cmos_read(CMOS_RTC_STB);
	
	// Convert BCD to binary
	if( !(stb & 0x04) )
	{
		s = (u8)((s & 0x0F) + ((s >> 4) * 10));
		m = (u8)((m & 0x0F) + ((m >> 4) * 10));
		h = (u8)(( (h&0x0F) + ((((h & 0x70) >> 4)*10)|(h&0x80) )));
		d = (u8)((d & 0x0F) + ((d >> 4) * 10));
		mo = (u8)((mo & 0x0F) + ((mo >> 4) * 10));
		y = (u8)((y & 0x0F) + ((y >> 4) * 10)) + 100;
	}
	
	// convert 12 hour to 24 hour
	if( !(stb & 0x02) && (h & 0x08) ){
		h = (u8)(((h & 0x7F) + 12) % 24);
	}
	
	// add a day for a leap year after february
	if( mo > 2 && ((y+1900)%4) == 0 ){
		d++;
	}
	
	time_t tm = s + m*60 + h*3600 + (d-1+mon_to_days[mo-1])*86400 + (y-70)*31536000 + ((y-69)/4)*86400 - ((y-1)/100)*86400 + ((y+299)/400)*86400;
	//time_t tm = s + (m*60) + (h*3600) + ((d+mon_to_days[mo-1])*86400) + ((mo-1)*2678400) + ((y+30)*31536000);
	
	return tm;
}