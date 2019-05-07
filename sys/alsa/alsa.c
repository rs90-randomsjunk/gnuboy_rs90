#include <stdio.h>
#include <stdint.h>
#include <alsa/asoundlib.h>
char *strdup();

#include "defs.h"
#include "pcm.h"
#include "rc.h"

struct pcm pcm;

static int dsp;
static char *dsp_device;
static int stereo = 1;
static int samplerate = 44100;
static int sound = 1;

static snd_pcm_t *handle;

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
	snd_pcm_hw_params_t *params;
	uint32_t val;
	int32_t dir = -1;
	snd_pcm_uframes_t frames;
	
	/* Open PCM device for playback. */
	int32_t rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);

	if (rc < 0)
		rc = snd_pcm_open(&handle, "plughw:0,0,0", SND_PCM_STREAM_PLAYBACK, 0);

	if (rc < 0)
		rc = snd_pcm_open(&handle, "plughw:0,0", SND_PCM_STREAM_PLAYBACK, 0);
		
	if (rc < 0)
		rc = snd_pcm_open(&handle, "plughw:1,0,0", SND_PCM_STREAM_PLAYBACK, 0);

	if (rc < 0)
		rc = snd_pcm_open(&handle, "plughw:1,0", SND_PCM_STREAM_PLAYBACK, 0);

	if (rc < 0)
	{
		fprintf(stderr, "unable to open PCM device: %s\n", snd_strerror(rc));
		return;
	}

	/* Allocate a hardware parameters object. */
	snd_pcm_hw_params_alloca(&params);

	/* Fill it in with default values. */
	rc = snd_pcm_hw_params_any(handle, params);
	if (rc < 0)
	{
		fprintf(stderr, "Error:snd_pcm_hw_params_any %s\n", snd_strerror(rc));
		return;
	}

	/* Set the desired hardware parameters. */

	/* Interleaved mode */
	rc = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (rc < 0)
	{
		fprintf(stderr, "Error:snd_pcm_hw_params_set_access %s\n", snd_strerror(rc));
		return;
	}

	/* Signed 16-bit little-endian format */
	rc = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
	if (rc < 0)
	{
		fprintf(stderr, "Error:snd_pcm_hw_params_set_format %s\n", snd_strerror(rc));
		return;
	}

	rc = snd_pcm_hw_params_set_channels(handle, params, 1+stereo);
	if (rc < 0)
	{
		fprintf(stderr, "Error:snd_pcm_hw_params_set_channels %s\n", snd_strerror(rc));
		return;
	}
	
	val = samplerate;
	rc=snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);
	if (rc < 0)
	{
		fprintf(stderr, "Error:snd_pcm_hw_params_set_rate_near %s\n", snd_strerror(rc));
		return;
	}

	/* Set period size to settings.aica.BufferSize frames. */
	frames = 1024;
	rc = snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);
	if (rc < 0)
	{
		fprintf(stderr, "Error:snd_pcm_hw_params_set_buffer_size_near %s\n", snd_strerror(rc));
		return;
	}
	frames *= 4;
	rc = snd_pcm_hw_params_set_buffer_size_near(handle, params, &frames);
	if (rc < 0)
	{
		fprintf(stderr, "Error:snd_pcm_hw_params_set_buffer_size_near %s\n", snd_strerror(rc));
		return;
	}

	/* Write the parameters to the driver */
	rc = snd_pcm_hw_params(handle, params);
	if (rc < 0)
	{
		fprintf(stderr, "Unable to set hw parameters: %s\n", snd_strerror(rc));
		return;
	}
	
	pcm.stereo = stereo;
	pcm.hz = samplerate;
	pcm.len = 1024;
	pcm.buf = malloc(pcm.len);
	
	return;
}

void pcm_close()
{
	if (handle)
	{
		snd_pcm_drain(handle);
		snd_pcm_close(handle);
	}
	
	if (pcm.buf) free(pcm.buf);
	memset(&pcm, 0, sizeof pcm);
	close(dsp);
}

int pcm_submit()
{
	int teller;
	short w;
	char soundbuffer[4096];
	long ret, len;
	
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
		
		len = pcm.pos/2;
		ret = snd_pcm_writei(handle, soundbuffer, len);
		while(ret != len) 
		{
			if (ret < 0) 
			{
				snd_pcm_prepare( handle );
			}
			else 
			{
				len -= ret;
			}
			ret = snd_pcm_writei(handle, soundbuffer, len);
		}
	}
	pcm.pos = 0;
	return 1;
}
