#ifndef _CMOS_H_
#define _CMOS_H_

#include "stewieos/kernel.h"

#define CMOS_PORT_REG 0x70
#define CMOS_PORT_DATA 0x71

#define CMOS_REG_DRIVE 0x10
#define CMOS_RTC_SEC 0x00
#define CMOS_RTC_MIN 0x02
#define CMOS_RTC_HRS 0x04
#define CMOS_RTC_WKD 0x06
#define CMOS_RTC_DAY 0x07
#define CMOS_RTC_MON 0x08
#define CMOS_RTC_YRS 0x09
#define CMOS_RTC_CEN 0x32 /* (maybe) */
#define CMOS_RTC_STA 0x0A /* Status Register A */
#define CMOS_RTC_STB 0x0B /* Status Register B */

#define RTC_FLAG_UPDATE (1<<7)

// Control NMI. These functions return the old state of the NMI masking bit
int cmos_nmi_set(int state);
#define cmos_nmi_enable() cmos_nmi_set(1)
#define cmos_nmi_disable() cmos_nmi_set(0)

u8 cmos_read(int reg);
void cmos_write(int reg, u8 v);

// Read the time from the RTC and convert it to a *NIX time
time_t rtc_read(void);

#endif