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
#include <sys/stat.h>

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
#include "hw.h"
#include "sys.h"

static char datfile[512];

struct fb fb;

static int use_yuv = -1;
static int fullscreen = 2;
static int use_altenter = -1;
static char Xstatus, Ystatus;

static byte *fakescreen = NULL;
static SDL_Surface *screen, *img_background, *backbuffer;

static uint32_t menu_triggers = 0;

static bool frameskip = 0;
static bool useframeskip = 0;
static bool showfps = 0;
static int fps = 0;
static int framecounter = 0;
static long time1 = 0;
static int volume = 30;
static int saveslot = 1;

extern bool emuquit;
static int startvolume=50;

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
 		SDL_FillRect( backbuffer, NULL, 0 );
		
		print_string("GnuBoy " __DATE__, TextWhite, 0, 56, 15, backbuffer->pixels);
		
		if (currentselection == 1) print_string("Continue", TextRed, 0, 5, 35, backbuffer->pixels);
		else  print_string("Continue", TextWhite, 0, 5, 35, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "Load State %d", saveslot);
		
		if (currentselection == 2) print_string(text, TextRed, 0, 5, 55, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 55, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "Save State %d", saveslot);
		
		if (currentselection == 3) print_string(text, TextRed, 0, 5, 75, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 75, backbuffer->pixels);
		
        if (currentselection == 4)
        {
			switch(fullscreen)
			{
				case 0:
					print_string("Scaling : Native", TextRed, 0, 5, 95, backbuffer->pixels);
				break;
				case 1:
					print_string("Scaling : Fullscreen", TextRed, 0, 5, 95, backbuffer->pixels);
				break;
				case 2:
					print_string("Scaling : 4:3 Upscale", TextRed, 0, 5, 95, backbuffer->pixels);
				break;
                case 3:
					print_string("Scaling : New 4:3", TextRed, 0, 5, 95, backbuffer->pixels);
				break;
			}
        }
        else
        {
			switch(fullscreen)
			{
				case 0:
					print_string("Scaling : Native", TextWhite, 0, 5, 95, backbuffer->pixels);
				break;
				case 1:
					print_string("Scaling : Fullscreen", TextWhite, 0, 5, 95, backbuffer->pixels);
				break;
				case 2:
					print_string("Scaling : 4:3 Upscale", TextWhite, 0, 5, 95, backbuffer->pixels);
				break;
                case 3:
					print_string("Scaling : New 4:3", TextWhite, 0, 5, 95, backbuffer->pixels);
				break;
			}
        }

		switch(useframeskip)
		{
			case 0:
				if (currentselection == 5) print_string("Frameskip : No", TextRed, 0, 5, 115, backbuffer->pixels);
				else print_string("Frameskip : No", TextWhite, 0, 5, 115, backbuffer->pixels);
			break;
			case 1:
				if (currentselection == 5) print_string("Frameskip : Yes", TextRed, 0, 5, 115, backbuffer->pixels);
				else print_string("Frameskip : Yes", TextWhite, 0, 5, 115, backbuffer->pixels);
			break;
		}
		
		if (currentselection == 6) print_string("Quit", TextRed, 0, 5, 135, backbuffer->pixels);
		else print_string("Quit", TextWhite, 0, 5, 135, backbuffer->pixels);

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
								fullscreen = 3;
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
                                if (fullscreen > 3)
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
                    if (fullscreen > 3)
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

		SDL_BlitSurface(backbuffer, NULL, screen, NULL);
		SDL_Flip(screen);
    }
    
    SDL_FillRect(screen, NULL, 0);
    SDL_Flip(screen);
    
    if (currentselection == 6)
    {
        emuquit = true;
	}
}

void vid_init()
{
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO))
	{
		printf("SDL: Couldn't initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}

	screen = SDL_SetVideoMode(240, 160, 16, SDL_SWSURFACE);
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
			switch(event.key.keysym.sym)
			{
				case SDLK_BACKSPACE:
				case SDLK_TAB:
					menu_triggers = 1;
				break;
				case SDLK_RETURN:
					pad_press(PAD_START);
				break;
				case SDLK_ESCAPE:
					pad_press(PAD_SELECT);
				break;
				case SDLK_UP:
					pad_press(PAD_UP);
				break;
				case SDLK_LEFT:
					pad_press(PAD_LEFT);
				break;
				case SDLK_RIGHT:
					pad_press(PAD_RIGHT);
				break;
				case SDLK_DOWN:
					pad_press(PAD_DOWN);
				break;
				case SDLK_LCTRL:
					pad_press(PAD_A);
				break;
				case SDLK_LALT:
					pad_press(PAD_B);
				break;
			}
			break;
		case SDL_KEYUP:
			switch(event.key.keysym.sym)
			{
				case SDLK_BACKSPACE:
				case SDLK_TAB:
					menu_triggers = 1;
				break;
				case SDLK_RETURN:
					pad_release(PAD_START);
				break;
				case SDLK_ESCAPE:
					pad_release(PAD_SELECT);
				break;
				case SDLK_UP:
					pad_release(PAD_UP);
				break;
				case SDLK_LEFT:
					pad_release(PAD_LEFT);
				break;
				case SDLK_RIGHT:
					pad_release(PAD_RIGHT);
				break;
				case SDLK_DOWN:
					pad_release(PAD_DOWN);
				break;
				case SDLK_LCTRL:
					pad_release(PAD_A);
				break;
				case SDLK_LALT:
					pad_release(PAD_B);
				break;
			}
			break;
		case SDL_QUIT:
			exit(1);
			break;
		default:
			break;
		}
	}

	if(menu_triggers == 1)
	{
		//memset(pcm.buf, 0, pcm.len);
		pcm_silence();
		menu();
		menu_triggers = 0;
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
	FILE *f;
	int vol;

	mkdir(sys_gethome(), 0755);
	
	startvolume = readvolume();
	snprintf(datfile, sizeof(datfile), "%s/gnuboy.cfg", sys_gethome());
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
	FILE *f;
	setvolume(startvolume);
	
	if (fakescreen) free(fakescreen);
	if (img_background) SDL_FreeSurface(img_background);
	if (backbuffer) SDL_FreeSurface(backbuffer);

	if (screen)
	{
		SDL_UnlockSurface(screen);
		SDL_FreeSurface(screen);
		SDL_Quit();

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
                    case 2: // scale 4:3
                        upscale_160x144_to_212x160((uint16_t* restrict)fakescreen, (uint16_t* restrict)screen->pixels);
						break;
                    case 3: // New scale 4:3
                        upscale_160x144_to_212x144((uint16_t* restrict)fakescreen, (uint16_t* restrict)screen->pixels);
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
