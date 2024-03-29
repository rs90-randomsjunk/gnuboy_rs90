#include <string.h>
#include <stdio.h>

#include "gnuboy.h"
#include "rtc.h"

gb_rtc_t rtc;

// Set in the far future for VBA-M support
#define RT_BASE 1893456000


void rtc_reset(bool hard)
{
	if (hard)
	{
		memset(&rtc, 0, sizeof(rtc));
	}
}

void rtc_latch(byte b)
{
	if ((rtc.latch ^ b) & b & 1)
	{
		rtc.regs[0] = rtc.s;
		rtc.regs[1] = rtc.m;
		rtc.regs[2] = rtc.h;
		rtc.regs[3] = rtc.d;
		rtc.regs[4] = rtc.flags;
	}
	rtc.latch = b & 1;
}

void rtc_write(byte b)
{
	MESSAGE_DEBUG("write %02X: %d\n", rtc.sel, b);

	switch (rtc.sel & 0xf)
	{
	case 0x8: // Seconds
		rtc.regs[0] = b;
		rtc.s = b % 60;
		break;
	case 0x9: // Minutes
		rtc.regs[1] = b;
		rtc.m = b % 60;
		break;
	case 0xA: // Hours
		rtc.regs[2] = b;
		rtc.h = b % 24;
		break;
	case 0xB: // Days (lower 8 bits)
		rtc.regs[3] = b;
		rtc.d = ((rtc.d & 0x100) | b) % 365;
		break;
	case 0xC: // Flags (days upper 1 bit, carry, stop)
		rtc.regs[4] = b;
		rtc.flags = b;
		rtc.d = ((rtc.d & 0xff) | ((b&1)<<9)) % 365;
		break;
	}
	rtc.dirty = 1;
}

void rtc_tick()
{
	if ((rtc.flags & 0x40))
		return; // rtc stop

	if (++rtc.ticks >= 60)
	{
		if (++rtc.s >= 60)
		{
			if (++rtc.m >= 60)
			{
				if (++rtc.h >= 24)
				{
					if (++rtc.d >= 365)
					{
						rtc.d = 0;
						rtc.flags |= 0x80;
					}
					rtc.h = 0;
				}
				rtc.m = 0;
			}
			rtc.s = 0;
		}
		rtc.ticks = 0;
	}
}

int rtc_save(const char *file)
{
	int ret = -1;
	int64_t rt = RT_BASE + (rtc.s + (rtc.m * 60) + (rtc.h * 3600) + (rtc.d * 86400));
	FILE *f;

	if (!file || !*file)
		return -1;

	if ((f = fopen(file, "wb")))
	{
		fwrite(&rtc.s, 4, 1, f);
		fwrite(&rtc.m, 4, 1, f);
		fwrite(&rtc.h, 4, 1, f);
		fwrite(&rtc.d, 4, 1, f);
		fwrite(&rtc.flags, 4, 1, f);
		for (int i = 0; i < 5; i++) {
			fwrite(&rtc.regs[i], 4, 1, f);
		}
		fwrite(&rt, 8, 1, f);
		fclose(f);
	}

	return ret;
}

int rtc_load(const char *file)
{
	int ret = -1;
	FILE *f;
	int64_t rt = 0;

	if (!file || !*file)
		return -1;

	if ((f = fopen(file, "rb")))
	{
		// Try to read old format first
		int tmp = fscanf(f, "%d %*d %d %02d %02d %02d %02d\n%*d\n",
			&rtc.flags, &rtc.d, &rtc.h, &rtc.m, &rtc.s, &rtc.ticks);

		if (tmp >= 5)
			return ret;

		fread(&rtc.s, 4, 1, f);
		fread(&rtc.m, 4, 1, f);
		fread(&rtc.h, 4, 1, f);
		fread(&rtc.d, 4, 1, f);
		fread(&rtc.flags, 4, 1, f);
		for (int i = 0; i < 5; i++) {
			fread(&rtc.regs[i], 4, 1, f);
		}
		fread(&rt, 8, 1, f);
		fclose(f);
	}

	return ret;
}
