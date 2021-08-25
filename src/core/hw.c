#include <string.h>

#include "defs.h"
#include "cpu.h"
#include "hw.h"
#include "regs.h"
#include "lcd.h"
#include "mem.h"
#include "fastmem.h"

struct hw hw;

/*
 * hw_interrupt changes the virtual interrupt lines included in the
 * specified mask to the values the corresponding bits in i take, and
 * in doing so, raises the appropriate bit of R_IF for any interrupt
 * lines that transition from low to high.
 */
void hw_interrupt(byte i, int level)
{
	if (level == 0)
	{
		hw.ilines &= ~i;
	}
	else if ((hw.ilines & i) == 0)
	{
		hw.ilines |= i;
		R_IF |= i; // Fire!

		if ((R_IE & i) != 0)
		{
			// Wake up the CPU when an enabled interrupt occurs
			// IME doesn't matter at this point, only IE
			cpu.halt = 0;
		}
	}
}


/*
 * hw_dma performs plain old memory-to-oam dma, the original dmg
 * dma. Although on the hardware it takes a good deal of time, the cpu
 * continues running during this mode of dma, so no special tricks to
 * stall the cpu are necessary.
 */

void hw_dma(byte b)
{
	int i;
	addr a;

	a = ((addr)b) << 8;
	for (i = 0; i < 160; i++, a++)
		lcd.oam.mem[i] = readb(a);
}

/* COMMENT A: NOTE this is partially superceded by COMMENT B below.
 * Beware was pretty sure that this HDMA implementation was incorrect, as when
 * he used it in bgb, it broke Pokemon Crystal (J). I tested it with this and
 * it seems to work fine, so until I find any problems with it, it's staying.
 * (Lord Nightmare)
 */


/* COMMENT B:
Timings for GDMA by /dalias/ by all means are accurate within few cycles, given
whatever we feed these values in takes a time to run expressed in double-speed
machine cycles as an argument. Whatever DMA-related issues remain, they are
likely in how intervals are applied and not in how they are calculated.

Had to replace cpu_timers() in GDMA section, don't know how but it was breaking
Shantae. Figured out the same thing happens to Pokemon Crystal and HDMA so I
also got HDMA init interval in place. Ghostbusters got fixed somehow somewhere
along the way.

Not sure how to be about now-missing lcd update. Note that DMA transfer now
completes immediately from LCDC perspective, which is far from accurate. Any
way we better get other bits straight to make sure they don't interfere before
trying to fix DMA any further.
*/


void hw_hdma()
{
	int cnt;
	addr sa;
	int da;

	sa = ((addr)R_HDMA1 << 8) | (R_HDMA2&0xf0);
	da = 0x8000 | ((int)(R_HDMA3&0x1f) << 8) | (R_HDMA4&0xf0);
	cnt = 16;
	while (cnt--)
		writeb(da++, readb(sa++));
		
	/* SEE COMMENT B ABOVE */
	/* NOTE: lcdc_trans() after a call to hw_hdma() will advance lcdc for us */
	div_advance(16 << cpu.speed);
	timer_advance(16 << cpu.speed);
	sound_advance(16);
	
	R_HDMA1 = sa >> 8;
	R_HDMA2 = sa & 0xF0;
	R_HDMA3 = 0x1F & (da >> 8);
	R_HDMA4 = da & 0xF0;
	R_HDMA5--;
	hw.hdma--;
}


void hw_hdma_cmd(byte c)
{
	int cnt;
	addr sa;
	int da;
	
	int advance;

	/* Begin or cancel HDMA */
	if ((hw.hdma|c) & 0x80)
	{
		hw.hdma = c;
		R_HDMA5 = c & 0x7f;
		
		/* SEE COMMENT B ABOVE */
		advance = 460 >> cpu.speed;
		div_advance(advance << cpu.speed);
		timer_advance(advance << cpu.speed);
		sound_advance(advance);
		
		/* FIXME: according to docs, hdma should not be started during hblank
		(Extreme Ghostbusters game does, but it also works without this line) */
		/* This breaks the look of some games like DKC. */
		/*if ((R_STAT&0x03) == 0x00) hw_hdma();*/
		return;
	}
	
	/* Perform GDMA */
	sa = ((addr)R_HDMA1 << 8) | (R_HDMA2&0xf0);
	da = 0x8000 | ((int)(R_HDMA3&0x1f) << 8) | (R_HDMA4&0xf0);
	cnt = ((int)c)+1;
	
	/* SEE COMMENT B ABOVE */
	/*cpu_timers((460>>cpu.speed)+cnt*16);*/ /*dalias*/
	advance = (460 >> cpu.speed) + cnt*16;
	div_advance(advance << cpu.speed);
	timer_advance(advance << cpu.speed);
	sound_advance(advance);
	
	cnt <<= 4;
	while (cnt--)
		writeb(da++, readb(sa++));
	R_HDMA1 = sa >> 8;
	R_HDMA2 = sa & 0xF0;
	R_HDMA3 = 0x1F & (da >> 8);
	R_HDMA4 = da & 0xF0;
	R_HDMA5 = 0xFF;
}


/*
 * pad_refresh updates the P1 register from the pad states, generating
 * the appropriate interrupts (by quickly raising and lowering the
 * interrupt line) if a transition has been made.
 */
void pad_refresh()
{
	byte oldp1 = R_P1;
	R_P1 &= 0x30;
	R_P1 |= 0xc0;
	if (!(R_P1 & 0x10)) R_P1 |= (hw.pad & 0x0F);
	if (!(R_P1 & 0x20)) R_P1 |= (hw.pad >> 4);
	R_P1 ^= 0x0F;
	if (oldp1 & ~R_P1 & 0x0F)
	{
		hw_interrupt(IF_PAD, 1);
		hw_interrupt(IF_PAD, 0);
	}
}


/*
 * pad_set updates the state of one or more buttons on the pad and calls
 * pad_refresh() to fire an interrupt if the pad changed.
 */
void pad_set(byte btn, int set)
{
	int new_pad = hw.pad;

	if (set)
		new_pad |= btn;
	else
		new_pad &= ~btn;

	if (hw.pad != new_pad)
	{
		hw.pad = new_pad;
		pad_refresh();
	}
}

void hw_reset()
{
	hw.ilines = 0;
	hw.hdma = 0;
	hw.pad = 0;

	memset(ram.ibank, 0, sizeof(ram.ibank));
	memset(ram.hi, 0, sizeof(ram.hi));
	R_P1 = 0xFF;
	R_LCDC = 0x91;
	R_BGP = 0xFC;
	R_OBP0 = 0xFF;
	R_OBP1 = 0xFF;
	R_SVBK = 0xF9;
	R_HDMA5 = 0xFF;
	R_VBK = 0xFE;
}
