#include <stdio.h>
#include <stdint.h>
#include <SDL/SDL.h>
#include "pcm.h"

struct pcm pcm;

static int sound = 1;
static int samplerate = 22050;
static int stereo = 1;
static volatile int audio_done;

SDL_mutex *sound_mutex;
SDL_cond *sound_cv;

static int readvolume()
{
	return 0;
}

static void pcm_silence()
{
	memset(pcm.buf, 0, pcm.len);	
}

static void setvolume(int involume)
{
}

static void audio_callback(void *blah, uint8_t  *stream, int len)
{
	SDL_LockMutex(sound_mutex);
	int teller = 0;
	signed short w;
	byte * bleh = (byte *) stream;
	for (teller = 0; teller < pcm.len; teller++)
	{
		w =  (uint16_t)((pcm.buf[teller] - 128) << 8);
		*bleh++ = w & 0xFF ;
		*bleh++ = w >> 8;
	}
	audio_done = 1;
	SDL_CondSignal(sound_cv);
	SDL_UnlockMutex(sound_mutex);
}


void pcm_init()
{
	int i;
	SDL_AudioSpec as;

	if (!sound) return;

	as.freq = samplerate;
	as.format = AUDIO_S16SYS;
	as.channels = 1 + stereo;
	as.samples = 1024;
	as.callback = audio_callback;
	as.userdata = 0;
	if (SDL_OpenAudio(&as, 0) == -1)
		return;

	pcm.hz = as.freq;
	pcm.stereo = as.channels - 1;
	pcm.len = as.size >> 1;
	pcm.buf = malloc(pcm.len);
	pcm.pos = 0;
	memset(pcm.buf, 0, pcm.len);

	sound_mutex = SDL_CreateMutex();
	sound_cv = SDL_CreateCond();

	SDL_PauseAudio(0);
}

int pcm_submit()
{
	if (!pcm.buf) return 0;
	if (pcm.pos < pcm.len) return 1;
	SDL_LockMutex(sound_mutex);
	while (!audio_done) SDL_CondWait(sound_cv, sound_mutex);
	audio_done = 0;
	pcm.pos = 0;
	SDL_CondSignal(sound_cv);
	SDL_UnlockMutex(sound_mutex);
	return 1;
}

void pcm_close()
{
	if (sound)
		SDL_CloseAudio();
}
