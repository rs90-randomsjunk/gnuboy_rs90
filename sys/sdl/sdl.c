/*
 * sdl.c
 * sdl interfaces -- based on svga.c
 *
 * (C) 2001 Damian Gryski <dgryski@uwaterloo.ca>
 * Joystick code contributed by David Lau
 * Sound code added by Laguna
 *
 * Licensed under the GPLv2, or later.
 */

#include <stdlib.h>
#include <stdio.h>

#ifndef WIN32
#include <linux/soundcard.h>
#include <sys/ioctl.h>
#endif
#include <fcntl.h>
#include <SDL/SDL.h>
#include "scaler.h"

#include "font_drawing.h"
#include "scaler.h"

#include "fb.h"
#include "input.h"
#include "rc.h"


struct fb fb;

static int use_yuv = -1;
static int fullscreen = 2;
static int use_altenter = -1;
static char Xstatus, Ystatus;

static byte *fakescreen = NULL;
SDL_Surface *screen;
SDL_Surface *img_background;

SDL_mutex *sound_mutex;
SDL_cond *sound_cv;

static bool frameskip = 0;
static bool useframeskip = 0;
static bool showfps = 0;
static int fps = 0;
static int framecounter = 0;
static long time1 = 0;
static int volume = 30;
static int saveslot = 1;
static int use_joy = 0;

static int vmode[3] = { 0, 0, 16 };
bool startpressed = false;
bool selectpressed = false;
extern bool emuquit;
char datfile[512];
static int startvolume=50;

rcvar_t vid_exports[] =
{
	RCV_VECTOR("vmode", &vmode, 3),
	RCV_BOOL("yuv", &use_yuv),
	RCV_BOOL("fullscreen", &fullscreen),
	RCV_BOOL("altenter", &use_altenter),
	RCV_END
};

rcvar_t joy_exports[] =
{
	RCV_BOOL("joy", &use_joy),
	RCV_END
};


/* keymap - mappings of the form { scancode, localcode } - from sdl/keymap.c */
extern int keymap[][2];

#ifndef NATIVE_AUDIO
#include "pcm.h"

struct pcm pcm;

static int sound = 1;
static int samplerate = 22050;
static int stereo = 1;
static volatile int audio_done;

rcvar_t pcm_exports[] =
{
	RCV_BOOL("sound", &sound),
	RCV_INT("stereo", &stereo),
	RCV_INT("samplerate", &samplerate),
	RCV_END
};


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
#endif


SDL_Surface *backbuffer;
extern void state_load(int n);
extern void state_save(int n);

void menu()
{
	char text[50];
    int16_t pressed = 0;
    int16_t currentselection = 1;

    SDL_Rect dstRect;
    SDL_Event Event;
    
    pressed = 0;
    currentselection = 1;
    
    while (((currentselection != 1) && (currentselection != 6)) || (!pressed))
    {
        pressed = 0;
 		SDL_FillRect( screen, NULL, 0 );
		
		print_string("GnuBoy", TextWhite, 0, 80, 15, screen->pixels);
		
		if (currentselection == 1) print_string("Continue", TextRed, 0, 5, 35, screen->pixels);
		else  print_string("Continue", TextWhite, 0, 5, 35, screen->pixels);
		
		snprintf(text, sizeof(text), "Load State %d", saveslot);
		
		if (currentselection == 2) print_string(text, TextRed, 0, 5, 55, screen->pixels);
		else print_string(text, TextWhite, 0, 5, 55, screen->pixels);
		
		snprintf(text, sizeof(text), "Save State %d", saveslot);
		
		if (currentselection == 3) print_string(text, TextRed, 0, 5, 75, screen->pixels);
		else print_string(text, TextWhite, 0, 5, 75, screen->pixels);
		
        if (currentselection == 4)
        {
			switch(fullscreen)
			{
				case 0:
					print_string("Scaling : Native", TextRed, 0, 5, 95, screen->pixels);
				break;
				case 1:
					print_string("Scaling : Fullscreen", TextRed, 0, 5, 95, screen->pixels);
				break;
				case 2:
					print_string("Scaling : Keep aspect", TextRed, 0, 5, 95, screen->pixels);
				break;
			}
        }
        else
        {
			switch(fullscreen)
			{
				case 0:
					print_string("Scaling : Native", TextWhite, 0, 5, 95, screen->pixels);
				break;
				case 1:
					print_string("Scaling : Fullscreen", TextWhite, 0, 5, 95, screen->pixels);
				break;
				case 2:
					print_string("Scaling : Keep aspect", TextWhite, 0, 5, 95, screen->pixels);
				break;
			}
        }

		switch(useframeskip)
		{
			case 0:
				if (currentselection == 5) print_string("Frameskip : No", TextRed, 0, 5, 115, screen->pixels);
				else print_string("Frameskip : No", TextWhite, 0, 5, 115, screen->pixels);
			break;
			case 1:
				if (currentselection == 5) print_string("Frameskip : Yes", TextRed, 0, 5, 115, screen->pixels);
				else print_string("Frameskip : Yes", TextWhite, 0, 5, 115, screen->pixels);
			break;
		}
		
		if (currentselection == 6) print_string("Quit", TextRed, 0, 5, 135, screen->pixels);
		else print_string("Quit", TextWhite, 0, 5, 135, screen->pixels);

        while (SDL_PollEvent(&Event))
        {
            if (Event.type == SDL_KEYDOWN)
            {
                switch(Event.key.keysym.sym)
                {
                    case SDLK_UP:
                        currentselection--;
                        if (currentselection == 0)
                            currentselection = 6;
                        break;
                    case SDLK_DOWN:
                        currentselection++;
                        if (currentselection == 7)
                            currentselection = 1;
                        break;
                    case SDLK_LCTRL:
                    case SDLK_LALT:
                    case SDLK_RETURN:
                        pressed = 1;
                        break;
                    case SDLK_LEFT:
                        switch(currentselection)
                        {
                            case 2:
                            case 3:
                                saveslot--;
                                if (saveslot < 1) saveslot = 0;
							break;
                            case 4:
							fullscreen--;
							if (fullscreen < 0)
								fullscreen = 2;
							break;
							case 5:
								useframeskip = 0;
							break;
                        }
                        break;
                    case SDLK_RIGHT:
                        switch(currentselection)
                        {
                            case 2:
                            case 3:
                                saveslot++;
								if (saveslot > 9)
									saveslot = 9;
							break;
                            case 4:
                                fullscreen++;
                                if (fullscreen > 2)
                                    fullscreen = 0;
							break;
							case 5:
								useframeskip = 1;
							break;
                        }
                        break;
					default:
					break;
                }
            }
            else if (Event.type == SDL_QUIT)
            {
				currentselection = 6;
			}
        }

        if (pressed)
        {
            switch(currentselection)
            {
				case 5:
					useframeskip = !useframeskip;
				break;
                case 4 :
                    fullscreen++;
                    if (fullscreen > 2)
                        fullscreen = 0;
                    break;
                case 2 :
					state_load(saveslot);
					currentselection = 1;
                    break;
                case 3 :
					state_save(saveslot);
					currentselection = 1;
				break;
				default:
				break;
            }
        }

		SDL_Flip(screen);
    }
    
    SDL_FillRect(screen, NULL, 0);
    SDL_Flip(screen);
    
    if (currentselection == 6)
    {
        emuquit = true;
	}
}

static int mapscancode(SDLKey sym)
{
	/* this could be faster:  */
	/*  build keymap as int keymap[256], then ``return keymap[sym]'' */

	int i;
	for (i = 0; keymap[i][0]; i++)
		if (keymap[i][0] == sym)
			return keymap[i][1];
	if (sym >= '0' && sym <= '9')
		return sym;
	if (sym >= 'a' && sym <= 'z')
		return sym;
	return 0;
}

void vid_init(char *s, char *s2)
{
	int flags;

	flags = SDL_HWSURFACE;
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO))
	{
		printf("SDL: Couldn't initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}

	screen = SDL_SetVideoMode(240, 160, 16, flags);
	if(!screen)
	{
		printf("SDL: can't set video mode: %s\n", SDL_GetError());
		exit(1);
	}

	SDL_ShowCursor(0);
	
	backbuffer = SDL_CreateRGBSurface(SDL_SWSURFACE, screen->w, screen->h, 16, 0, 0, 0, 0);

	fakescreen = calloc(160*144, 2);

	fb.w = 160;
	fb.h = 144;
	fb.ptr = fakescreen;
	fb.pelsize = 2;
	fb.pitch = 320;
	fb.indexed = 0;
	fb.cc[0].r = 3;
	fb.cc[1].r = 2;
	fb.cc[2].r = 3;
	fb.cc[0].l = 11;
	fb.cc[1].l = 5;
	fb.cc[2].l = 0;

	SDL_FillRect(screen,NULL,0);
	SDL_Flip(screen);

	fb.enabled = 1;
	fb.dirty = 0;
}

extern void pcm_silence();


void ev_poll()
{
	event_t ev;
	SDL_Event event;

	while (SDL_PollEvent(&event))
	{
		switch(event.type)
		{
		case SDL_ACTIVEEVENT:
			if (event.active.state == SDL_APPACTIVE)
				fb.enabled = event.active.gain;
			break;
		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_BACKSPACE || event.key.keysym.sym == SDLK_TAB)
			{
				startpressed = true;
				selectpressed = true;
			}

			if (event.key.keysym.sym == SDLK_RETURN)
			{
				startpressed = true;
			}

			if (event.key.keysym.sym == SDLK_ESCAPE)
			{
				selectpressed = true;
			}
			ev.type = EV_PRESS;
			ev.code = mapscancode(event.key.keysym.sym);
			ev_postevent(&ev);
			break;
		case SDL_KEYUP:
			if (event.key.keysym.sym == SDLK_RETURN)
			{
				startpressed = false;
			}
			if (event.key.keysym.sym == SDLK_ESCAPE)
			{
				selectpressed = false;
			}
			ev.type = EV_RELEASE;
			ev.code = mapscancode(event.key.keysym.sym);
			ev_postevent(&ev);
			break;
		case SDL_QUIT:
			exit(1);
			break;
		default:
			break;
		}
	}

	if((startpressed && selectpressed))
	{
		event_t ev;
		ev.type = EV_RELEASE;
		ev.code = mapscancode(SDLK_RETURN);
		ev_postevent(&ev);
		ev.type = EV_RELEASE;
		ev.code = mapscancode(SDLK_ESCAPE);
		ev_postevent(&ev);
		//memset(pcm.buf, 0, pcm.len);
		pcm_silence();
		menu();
		startpressed = false;
		selectpressed = false;
		if (emuquit)
		{
			pcm_silence();
		}
		
		SDL_FillRect(screen, NULL, 0 );
		SDL_Flip(screen);

	}

}

void vid_setpal(int i, int r, int g, int b)
{
}

void vid_preinit()
{
	static char homedir[512];
	FILE *f;
	int vol;
	
	snprintf(homedir, sizeof(homedir), "%s/.gnuboy", getenv("HOME"));
	mkdir(homedir, 0777);
	
	startvolume = readvolume();
	snprintf(homedir, sizeof(homedir), "%s/.gnuboy/gnuboy.cfg", getenv("HOME"));
	snprintf(datfile, sizeof(datfile), "%s", homedir);
	f = fopen(datfile,"rb");
	if(f)
	{
		fread(&fullscreen,sizeof(int),1,f);
		fread(&volume,sizeof(int),1,f);
		fread(&saveslot,sizeof(int),1,f);
		fread(&useframeskip,sizeof(bool),1,f);
		fread(&showfps,sizeof(bool),1,f);
		fclose(f);
	}
	else
	{
		fullscreen = 2;
		volume = 75;
		saveslot = 1;
		useframeskip = false;
	}
}

void vid_close()
{
	setvolume(startvolume);
	if(fakescreen);
		free(fakescreen);
		
	if (img_background)
		SDL_FreeSurface(img_background);
		
	if (backbuffer)
		SDL_FreeSurface(backbuffer);

	if(screen);
	{
		SDL_UnlockSurface(screen);
		SDL_FreeSurface(screen);
		SDL_Quit();

		FILE *f;
		f = fopen(datfile,"wb");
		if(f)
		{
			fwrite(&fullscreen,sizeof(int),1,f);
			fwrite(&volume,sizeof(int),1,f);
			fwrite(&saveslot,sizeof(int),1,f);
			fwrite(&useframeskip,sizeof(bool),1,f);
			fwrite(&showfps,sizeof(bool),1,f);
			fclose(f);
		}
	}
	fb.enabled = 0;
}

void vid_settitle(char *title)
{
	SDL_WM_SetCaption(title, title);
}


void vid_begin()
{
	if (!emuquit)
	{
		if ((!frameskip) ||  (!useframeskip))
		{
			if (SDL_LockSurface(screen) == 0)
			{
				switch(fullscreen) 
				{
					case 1: // normal fullscreen
						bitmap_scale(0,0,160,144,240,160, 160, 0, (uint16_t* restrict)fakescreen,(uint16_t* restrict)screen->pixels);
						break;
					case 2: // aspect ratio
						bitmap_scale(0,0,160,144, 178, 160, 160, screen->w-178, (uint16_t* restrict)fakescreen,(uint16_t* restrict)screen->pixels+(screen->h-160)/2*screen->w + (screen->w-178)/2);
						break;
					default: // native resolution
						bitmap_scale(0,0,160,144,160,144, 160, screen->w-160, (uint16_t* restrict)fakescreen,(uint16_t* restrict)screen->pixels+(screen->h-144)/2*screen->w + (screen->w-160)/2);
						break;
				}
				SDL_UnlockSurface(screen);
			}
			SDL_Flip(screen);
		}
		frameskip = !frameskip;
	}
}

void vid_end()
{
}
