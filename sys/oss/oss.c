

#include <stdlib.h>
#include <string.h>
char *strdup();
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef IS_FBSD
#include "machine/soundcard.h"
#define DSP_DEVICE "/dev/dsp"
#endif

#ifdef IS_OBSD
#include "soundcard.h"
#define DSP_DEVICE "/dev/sound"
#endif

#ifdef IS_LINUX
#include <sys/soundcard.h>
#define DSP_DEVICE "/dev/dsp"
#endif

#include "defs.h"
#include "pcm.h"
#include "rc.h"

/* FIXME - all this code is VERY basic, improve it! */


struct pcm pcm;

static int dsp;
static char *dsp_device;
static int stereo = 1;
static int samplerate = 44100;
static int sound = 1;

rcvar_t pcm_exports[] =
{
	RCV_BOOL("sound", &sound),
	RCV_INT("stereo", &stereo),
	RCV_INT("samplerate", &samplerate),
	RCV_STRING("oss_device", &dsp_device),
	RCV_END
};

void pcm_silence()
{
	memset(pcm.buf, 0, pcm.len);	
}

void setvolume(int involume)
{
}

int readvolume()
{
	return 0;
}

void pcm_init()
{
	int n;

	if (!sound)
	{
		pcm.hz = 11025;
		pcm.len = 4096;
		pcm.buf = malloc(pcm.len);
		pcm.pos = 0;
		dsp = -1;
		return;
	}

	if (!dsp_device) dsp_device = strdup(DSP_DEVICE);
	dsp = open(dsp_device, O_WRONLY);

	n = 0x80012;
	ioctl(dsp, SNDCTL_DSP_SETFRAGMENT, &n);
	n = AFMT_S16_LE;
	ioctl(dsp, SNDCTL_DSP_SETFMT, &n);
	n = stereo;
	ioctl(dsp, SNDCTL_DSP_STEREO, &n);
	pcm.stereo = n;
	n = samplerate;
	ioctl(dsp, SNDCTL_DSP_SPEED, &n);
	pcm.hz = n;
	pcm.len = 2048;
	pcm.buf = malloc(pcm.len);
}

void pcm_close()
{
	if (pcm.buf) free(pcm.buf);
	memset(&pcm, 0, sizeof pcm);
	close(dsp);
}

int pcm_submit()
{
	int teller;
	short w;
	char soundbuffer[4096];
	if (dsp < 0)
	{
		pcm.pos = 0;
		return 0;
	}
	if (pcm.pos < pcm.len)
        return 1;

	if (pcm.buf)
	{
		byte * bleh = (byte *) soundbuffer;
		for (teller = 0; teller < pcm.pos; teller++)
		{
			w = (unsigned short)((pcm.buf[teller] - 128) << 8);
			*bleh++ = w & 0xFF ;
			*bleh++ = w >> 8;
		}
		write(dsp, soundbuffer,pcm.pos);
	}
	pcm.pos = 0;
	return 1;
}






