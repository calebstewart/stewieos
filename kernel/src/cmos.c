#include "cmos.h"
#include "error.h"

static int g_nmi_state = 1;
unsigned int sec_until_mon[12] = {
	0,		/* January (no months before it) */
	2678400,	/* February (seconds in January) ASSUMED 28 Days! Add 86400 to each one after this for leap year! */
	5097600,	/* March (seconds in january and february) */
	7776000,	/* April (seconds before march + seconds in march) */
	10368000,	/* May (seconds in before April + seconds in April) */
	13046400,	/* June */
	15638400,	/* July */
	18316800,	/* August */
	20995200,	/* September */
	23587200,	/* October */
	26265600,	/* November */
	28857600	/* December */
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
	// wait for update in progress flag to go from 1 -> 0
	while( !(cmos_read(CMOS_RTC_STA) & RTC_FLAG_UPDATE) );
	while( (cmos_read(CMOS_RTC_STA) & RTC_FLAG_UPDATE) );
	// Read all parameters
	u8 s = cmos_read(CMOS_RTC_SEC);
	u8 m = cmos_read(CMOS_RTC_MIN);
	u8 h = cmos_read(CMOS_RTC_HRS);
	u8 d = cmos_read(CMOS_RTC_DAY);
	u8 mo = cmos_read(CMOS_RTC_MON);
	int y = cmos_read(CMOS_RTC_YRS) + 30; // we assume the RTC reports a year in the 2000s, since I wrote this in 2014...
	
	printk("%d %d %d %02d:%02d:%02d\n", y+1970,mo,d,h,m,s);
	
	// google said a year was 3.15569e7... so I guess I will blindly ignore leap years... :(
	time_t tm = s + (m*60) + (h*3600) + (d*86400) + sec_until_mon[mo] + (y*31556900);
	
	return tm;
}