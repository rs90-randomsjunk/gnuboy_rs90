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
#include <unistd.h>

#ifndef WIN32
#include <linux/soundcard.h>
#include <sys/ioctl.h>
#endif
#include <fcntl.h>
#include <SDL/SDL.h>
#include "scaler.h"

#include "font_drawing.h"
#include "scaler.h"

#include "hw.h"
#include "lcd.h"
#include "sound.h"
#include "sound_output.h"
#include "save.h"
#include "rtc.h"

bool emuquit = 0;

static char datfile[512];

//struct fb fb;

static int fullscreen = 0;
static SDL_Surface *fakescreen = NULL;
SDL_Surface *screen, *backbuffer;

static uint32_t menu_triggers = 0;

static bool frameskip = 0;
static bool useframeskip = 0;
static bool showfps = 0;
static int fps = 0;
static int framecounter = 0;
static long time1 = 0;
static int volume = 30;
static int saveslot = 1;
static int colorpalette = 0;

static int startvolume=50;

extern char *sys_gethome();
extern void sys_initpath();

extern char homedir[128];
extern char savesdir[192];
extern char statesdir[192];

void die(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	//loader_unload();
	//shutdown();
	exit(1);
}

static char *base(char *s)
{
	char *p;
	p = strrchr(s, '/');
	if (p) return p+1;
	return s;
}

static char *basefolder(char *s)
{
	char *p,*ns;
	p = strrchr(s, '/');
	if (p)
	{
		ns = malloc(p-s+2);
		memset(ns, 0, p-s+2);
		strncpy(ns,s,(p-s+1));
		return ns;
	}
	ns = malloc(2);
	sprintf(ns, ".");
	return ns;
}

extern void cleanup();

void menu()
{
	char tmp_save_dir[256];
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
		
		print_string("GnuBoy " __DATE__, TextWhite, 0, 48, 15, backbuffer->pixels);
		
        print_string("Continue", (currentselection == 1 ? TextRed : TextWhite) , 0, 5, 35, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "Load State: %d", saveslot);
		print_string(text,  (currentselection == 2 ? TextRed : TextWhite), 0, 5, 50, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "Save State: %d", saveslot);
		print_string(text,  (currentselection == 3 ? TextRed : TextWhite), 0, 5, 65, backbuffer->pixels);
		
        char scaling_mode[4][32] = {
            "Scaling   : Native",
            "Scaling   : 3:2 (Full)", 
            "Scaling   : 4:3", 
            "Scaling   : 3:2 (Alt)"
        };
        print_string(scaling_mode[fullscreen],  (currentselection == 4 ? TextRed : TextWhite), 0, 5, 80, backbuffer->pixels);
 
        char colorpalette_mode[5][32] = {
            "Mono Color: GBC",
            "Mono Color: Green shades",
            "Mono Color: Black & White",
            "Mono Color: Green & White",
            "Mono Color: Black & Red"
        };
        print_string(colorpalette_mode[colorpalette], (currentselection == 5 ? TextRed : TextWhite), 0, 5, 95, backbuffer->pixels);
        
		print_string("Quit", (currentselection == 6 ? TextRed : TextWhite), 0, 5, 110, backbuffer->pixels);

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
                    //case SDLK_RETURN:
                        pressed = 1;
                        break;
                    case SDLK_LALT:  //return to game
                        currentselection = 1;
                        pressed = 1;
                        break;
                    case SDLK_LEFT:
                        switch(currentselection)
                        {
                            case 2:
                            case 3:
                                if (saveslot > 0) saveslot--;
							break;
                            case 4:
                                if (fullscreen > 0) fullscreen--;
							break;
                            case 5:
                                if (colorpalette > 0) colorpalette --;
                            break;
                        }
                        break;
                    case SDLK_RIGHT:
                        switch(currentselection)
                        {
                            case 2:
                            case 3:
                                if (saveslot < 9) saveslot++;
							break;
                            case 4:
                                if (fullscreen < 3) fullscreen++;
							break;
                            case 5:
                                if (colorpalette < 4) colorpalette++;
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
                case 4 :
                    fullscreen++;
                    if (fullscreen > 3)
                        fullscreen = 3;
                    break;
                case 2 :
					snprintf(tmp_save_dir, sizeof(tmp_save_dir), "%s/%s_%d.sts", statesdir, cart.name, saveslot);
					state_load(tmp_save_dir);
					currentselection = 1;
                    break;
                case 3 :
					snprintf(tmp_save_dir, sizeof(tmp_save_dir), "%s/%s_%d.sts", statesdir, cart.name, saveslot);
					state_save(tmp_save_dir);
					currentselection = 1;
                    break;
				default:
                    break;
            }
        }

		SDL_BlitSurface(backbuffer, NULL, screen, NULL);
		SDL_Flip(screen);
    }
    
	lcd.out.colorize = colorpalette;
	lcd_rebuildpal();
    
    SDL_FillRect(screen, NULL, 0);
    SDL_Flip(screen);
    SDL_FillRect(screen, NULL, 0);
    SDL_Flip(screen);
    SDL_FillRect(screen, NULL, 0);
    SDL_Flip(screen);
    
    if (currentselection == 6)
    {
        emuquit = true;
		snprintf(tmp_save_dir, sizeof(tmp_save_dir), "%s/%s.sav", savesdir, cart.name);
		sram_save(tmp_save_dir);
		snprintf(tmp_save_dir, sizeof(tmp_save_dir), "%s/%s.rtc", savesdir, cart.name);
		rtc_save(tmp_save_dir);
	}
}

void vid_init()
{
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO))
	{
		printf("SDL: Couldn't initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}

	screen = SDL_SetVideoMode(0, 0, 16, SDL_HWSURFACE
	#ifdef SDL_TRIPLEBUF
	| SDL_TRIPLEBUF
	#else
	| SDL_DOUBLEBUF
	#endif
	);
	if(!screen)
	{
		printf("SDL: can't set video mode: %s\n", SDL_GetError());
		exit(1);
	}

	SDL_ShowCursor(0);
	
	backbuffer = SDL_CreateRGBSurface(SDL_HWSURFACE, screen->w, screen->h, 16, 0, 0, 0, 0);
	fakescreen = SDL_CreateRGBSurface(SDL_HWSURFACE, 160, 144, 16, 0, 0, 0, 0);

	lcd.out.format = GB_PIXEL_565_LE;
	lcd.out.buffer = fakescreen->pixels;
	lcd.out.enabled = 1;
	SDL_FillRect(screen,NULL,0);
	SDL_Flip(screen);
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
				lcd.out.enabled = event.active.gain;
			break;
		case SDL_KEYDOWN:
			switch(event.key.keysym.sym)
			{
				case SDLK_BACKSPACE:
				case SDLK_TAB:
					menu_triggers = 1;
				break;
				case SDLK_RETURN:
					hw_setpad(PAD_START, 1);
				break;
				case SDLK_ESCAPE:
					hw_setpad(PAD_SELECT, 1);
				break;
				case SDLK_UP:
					hw_setpad(PAD_UP, 1);
				break;
				case SDLK_LEFT:
					hw_setpad(PAD_LEFT, 1);
				break;
				case SDLK_RIGHT:
					hw_setpad(PAD_RIGHT, 1);
				break;
				case SDLK_DOWN:
					hw_setpad(PAD_DOWN, 1);
				break;
				case SDLK_LCTRL:
				case SDLK_SPACE:
					hw_setpad(PAD_A, 1);
				break;
				case SDLK_LALT:
				case SDLK_LSHIFT:
					hw_setpad(PAD_B, 1);
				break;
			}
			break;
		case SDL_KEYUP:
			switch(event.key.keysym.sym)
			{
				case SDLK_BACKSPACE:
				case SDLK_TAB:
				case SDLK_HOME:
					menu_triggers = 1;
				break;
				case SDLK_RETURN:
					hw_setpad(PAD_START, 0);
				break;
				case SDLK_ESCAPE:
					hw_setpad(PAD_SELECT, 0);
				break;
				case SDLK_UP:
					hw_setpad(PAD_UP, 0);
				break;
				case SDLK_LEFT:
					hw_setpad(PAD_LEFT, 0);
				break;
				case SDLK_RIGHT:
					hw_setpad(PAD_RIGHT, 0);
				break;
				case SDLK_DOWN:
					hw_setpad(PAD_DOWN, 0);
				break;
				case SDLK_LCTRL:
				case SDLK_SPACE:
					hw_setpad(PAD_A, 0);
				break;
				case SDLK_LALT:
				case SDLK_LSHIFT:
					hw_setpad(PAD_B, 0);
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
		SDL_PauseAudio(1);
		pcm_silence();
		pcm_silence();
		SDL_Delay(60);
		menu();
		menu_triggers = 0;
		if (emuquit)
		{
			pcm_silence();
		}
		SDL_FillRect(screen, NULL, 0 );
		SDL_Flip(screen);
		SDL_PauseAudio(0);
	}
    

}

static void vid_preinit()
{
	FILE *f;
	int vol;
	
	snprintf(datfile, sizeof(datfile), "%s/.gnuboy", getenv("HOME"));
	mkdir(datfile, 0755);
	
	startvolume = readvolume();
	snprintf(datfile, sizeof(datfile), "%s/.gnuboy/gnuboy.cfg", getenv("HOME"));
	f = fopen(datfile,"rb");
	if(f)
	{
		fread(&fullscreen,sizeof(int),1,f);
		fread(&volume,sizeof(int),1,f);
		fread(&saveslot,sizeof(int),1,f);
		fread(&useframeskip,sizeof(bool),1,f);
		fread(&showfps,sizeof(bool),1,f);
        fread(&colorpalette,sizeof(int),1,f);
		fclose(f);
	}
	else
	{
		fullscreen = 0;
		volume = 75;
		saveslot = 0;
		useframeskip = false;
	}
}

static void vid_close()
{
	FILE *f;
	setvolume(startvolume);
	
	if (fakescreen) SDL_FreeSurface(fakescreen);
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
            fwrite(&colorpalette,sizeof(int),1,f);
			fclose(f);
            sync();
		}
	}
	lcd.out.enabled = 0;
}


void vid_begin()
{
	SDL_Rect dst_dest, src_dest;
	/* If screen width is 240 then use RS-90 codepath, otherwise use Generic. */
	if (screen->w == 240)
	{
		switch(fullscreen) 
		{
			case 1: // 3:2(Full)
				upscale_160x144_to_240x160((uint16_t* restrict)fakescreen->pixels, (uint16_t* restrict)screen->pixels);
			break;
			case 2: // scale 4:3
				upscale_160x144_to_212x160((uint16_t* restrict)fakescreen->pixels, (uint16_t* restrict)screen->pixels);
			break;
			case 3: // 3:2(Alt)
				upscale_160x144_to_212x144((uint16_t* restrict)fakescreen->pixels, (uint16_t* restrict)screen->pixels);
			break;
			default: // native resolution
				//bitmap_scale(0,0,160,144,160,144, 160, screen->w-160, (uint16_t* restrict)fakescreen->pixels,(uint16_t* restrict)screen->pixels+(screen->h-144)/2*screen->w + (screen->w-160)/2);
				src_dest.x = (screen->w-160)/2;
				src_dest.y = (screen->h-144)/2;
				src_dest.w = 160;
				src_dest.h = 144;
				SDL_BlitSurface(fakescreen, NULL, screen, &src_dest);
			break;
		}
	}
	else
	{
		switch(fullscreen)
		{
			case 1: // 3:2(Full)
			case 3:
				SDL_SoftStretch(fakescreen, NULL, screen, NULL);
			break;
			case 2: // scale 4:3
				src_dest.w = screen->w * 0.834375f;
				src_dest.h = screen->h;
				src_dest.x = (screen->w-src_dest.w)/2;
				src_dest.y = 0;
				SDL_SoftStretch(fakescreen, NULL, screen, &src_dest);
				//upscale_160x144_to_212x160((uint16_t* restrict)fakescreen->pixels, (uint16_t* restrict)screen->pixels);
			break;
			default: // native resolution
				//bitmap_scale(0,0,160,144,160,144, 160, screen->w-160, (uint16_t* restrict)fakescreen->pixels,(uint16_t* restrict)screen->pixels+(screen->h-144)/2*screen->w + (screen->w-160)/2);
				src_dest.x = (screen->w-160)/2;
				src_dest.y = (screen->h-144)/2;
				src_dest.w = 160;
				src_dest.h = 144;
				SDL_BlitSurface(fakescreen, NULL, screen, &src_dest);
			break;	
		}
	}
	SDL_Flip(screen);
}



int main(int argc, char *argv[])
{
	char* rom;
	char tmp_save_dir[192];
	rom = strdup(argv[1]);
	
	sys_initpath();
	
	gnuboy_load_rom(rom);
	
	/* If we have special perms, drop them ASAP! */
	vid_preinit();
	vid_init();
	pcm_init();
	
	gnuboy_init();
	gnuboy_reset(1);
	
	/* Only load the SRAM after we reset the memory. */
    snprintf(tmp_save_dir, sizeof(tmp_save_dir), "%s/%s.sav", savesdir, cart.name);
    sram_load(tmp_save_dir);
    snprintf(tmp_save_dir, sizeof(tmp_save_dir), "%s/%s.rtc", savesdir, cart.name);
    rtc_load(tmp_save_dir);
	
	gnuboy_run(1);
	
	gnuboy_free_rom();
	pcm_close();
	vid_close();

	/* never reached */
	return 0;
}

