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

#include "fb.h"
#include "input.h"
#include "hw.h"
#include "sys.h"
#include "lcd.h"
#include "sound.h"

static char datfile[512];

struct fb fb;

static int use_yuv = -1;
static int fullscreen = 0;
static int use_altenter = -1;
static char Xstatus, Ystatus;

static SDL_Surface *screen, *backbuffer, *fakescreen;

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

extern bool emuquit;
static int startvolume=50;

#define UINT16_16(val) ((uint32_t)(val * (float)(1<<16)))
static const uint32_t YUV_MAT[3][3] = {
	{UINT16_16(0.2999f),   UINT16_16(0.587f),    UINT16_16(0.114f)},
	{UINT16_16(0.168736f), UINT16_16(0.331264f), UINT16_16(0.5f)},
	{UINT16_16(0.5f),      UINT16_16(0.418688f), UINT16_16(0.081312f)}
};
static uint8_t drm_palette[3][256];

extern int dmg_pal[4][4];

extern int state_load(int n);
extern int state_save(int n);

#ifndef SDL_TRIPLEBUF
#define SDL_TRIPLEBUF SDL_DOUBLEBUF
#endif

#ifndef SDL_YUV444
#define SDL_YUV444 0
#endif

static void SetVideo(void)
{
	uint16_t i;
	if (screen) SDL_FreeSurface(screen);
	screen = SDL_SetVideoMode(160, 144, 24, SDL_HWSURFACE | SDL_TRIPLEBUF | SDL_YUV444);
	fb.w = 160;
	fb.h = 144;
	fb.ptr = (uint8_t*)fakescreen->pixels;
	fb.pelsize = 1;
	fb.pitch = fakescreen->pitch;
	fb.indexed = 1;
	fb.cc[0].r = 0;
	fb.cc[1].r = 0;
	fb.cc[2].r = 0;
	fb.cc[0].l = 0;
	fb.cc[1].l = 0;
	fb.cc[2].l = 0;
}

void menu()
{
	char text[50];
    int16_t pressed = 0;
    int16_t currentselection = 1;
    int fullscreen_old;

    SDL_Rect dstRect;
    SDL_Event Event;
    
    pressed = 0;
    currentselection = 1;
    
    int colorpalette_old = colorpalette;
    if (fullscreen > 1) fullscreen = 1;
    fullscreen_old = fullscreen;
    
	if (screen) SDL_FreeSurface(screen);
    screen = SDL_SetVideoMode(240, 160, 16, SDL_HWSURFACE | SDL_TRIPLEBUF);
    
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

        char frameskip_mode[2][32] = {
            "Frameskip : No",
            "Frameskip : Yes(NotRecommend)"
        };
        print_string(frameskip_mode[useframeskip], (currentselection == 4 ? TextRed : TextWhite), 0, 5, 80, backbuffer->pixels);
		
        char colorpalette_mode[5][32] = {
            "Mono Color: Gray",
            "Mono Color: Black",
            "Mono Color: Green",
            "Mono Color: Black & Gray",
            "Mono Color: Black & Green"
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
								useframeskip = 0;
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
							case 4:
								useframeskip = 1;
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
				case 4:
					useframeskip = !useframeskip;
                    break;
                case 2 :
					currentselection = 1;
					state_load(saveslot);
                    break;
                case 3 :
					currentselection = 1;
					state_save(saveslot);
                    break;
				default:
                    break;
            }
        }

		SDL_BlitSurface(backbuffer, NULL, screen, NULL);
		SDL_Flip(screen);
    }
    
    #define DEF_PAL1 {0xf8f8f8,0xa8a8a8,0x606060,0x000000} //clear black
    #define DEF_PAL2 {0x98d0e0,0x68a0b0,0x60707C,0x2C3C3C} //gray(Default)
    #define DEF_PAL3 {0xd5f3ef,0x7ab6a3,0x3b6137,0x161c04} //green
    #define DEF_PAL4 {0x80f880,0x00e210,0x608010,0x204010} //light green
    
    int paltmp0[4][4] = { DEF_PAL2, DEF_PAL2, DEF_PAL2, DEF_PAL2 }; //default
    int paltmp1[4][4] = { DEF_PAL1, DEF_PAL1, DEF_PAL1, DEF_PAL1 }; //Black
    int paltmp2[4][4] = { DEF_PAL4, DEF_PAL4, DEF_PAL4, DEF_PAL4 }; //Green
    int paltmp3[4][4] = { DEF_PAL2, DEF_PAL2, DEF_PAL1, DEF_PAL1 }; //Black+Gray
    int paltmp4[4][4] = { DEF_PAL4, DEF_PAL4, DEF_PAL1, DEF_PAL1 }; // Black+Green

    switch(colorpalette){
        case 0:
            for(int i = 0; i < 4; i++)
                for(int j =0; j < 4; j++)
                    dmg_pal[i][j] = paltmp0[i][j];
            break;
        case 1:
            for(int i = 0; i < 4; i++)
                for(int j =0; j < 4; j++)
                    dmg_pal[i][j] = paltmp1[i][j];
            break;
        case 2:
            for(int i = 0; i < 4; i++)
                for(int j =0; j < 4; j++)
                    dmg_pal[i][j] = paltmp2[i][j];
            break;
        case 3:
            for(int i = 0; i < 4; i++)
                for(int j =0; j < 4; j++)
                    dmg_pal[i][j] = paltmp3[i][j];
            break;
        case 4:
            for(int i = 0; i < 4; i++)
                for(int j =0; j < 4; j++)
                    dmg_pal[i][j] = paltmp4[i][j];
            break;
    }
    
    if (currentselection == 6)
        emuquit = true;
	else
	{
		SetVideo();
		
		pal_dirty();
		
		SDL_FillRect(screen, NULL, 0);
		SDL_Flip(screen);
		SDL_FillRect(screen, NULL, 0);
		SDL_Flip(screen);
		SDL_FillRect(screen, NULL, 0);
		SDL_Flip(screen);
	}
}


void vid_init()
{
	uint_fast16_t i;
	if (SDL_Init(SDL_INIT_VIDEO))
	{
		printf("SDL: Couldn't initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}

	SDL_ShowCursor(0);
	fakescreen = SDL_CreateRGBSurface(SDL_HWSURFACE, 160, 144, 8, 0, 0, 0, 0);
	backbuffer = SDL_CreateRGBSurface(SDL_HWSURFACE, 240, 160, 16, 0, 0, 0, 0);
	SetVideo();
	if(!screen)
	{
		printf("SDL: can't set video mode: %s\n", SDL_GetError());
		exit(1);
	}

	for(i=0;i<256;i++)
	{
		drm_palette[0][i] = ( ( UINT16_16(  0) + YUV_MAT[0][0] * 0 + YUV_MAT[0][1] * 0 + YUV_MAT[0][2] * 0) >> 16 );
		drm_palette[1][i] = ( ( UINT16_16(128) - YUV_MAT[1][0] * 0 - YUV_MAT[1][1] * 0 + YUV_MAT[1][2] * 0) >> 16 );
		drm_palette[2][i] = ( ( UINT16_16(128) + YUV_MAT[2][0] * 0 - YUV_MAT[2][1] * 0 - YUV_MAT[2][2] * 0) >> 16 );	
	}
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
	//colors[i].r=r;
	//colors[i].g=g;
	//colors[i].b=b;
	//SDL_SetPalette(screen, SDL_LOGPAL|SDL_PHYSPAL, colors, 0, 256);
	drm_palette[0][i] = ( ( UINT16_16(  0) + YUV_MAT[0][0] * r + YUV_MAT[0][1] * g + YUV_MAT[0][2] * b) >> 16 );
	drm_palette[1][i] = ( ( UINT16_16(128) - YUV_MAT[1][0] * r - YUV_MAT[1][1] * g + YUV_MAT[1][2] * b) >> 16 );
	drm_palette[2][i] = ( ( UINT16_16(128) + YUV_MAT[2][0] * r - YUV_MAT[2][1] * g - YUV_MAT[2][2] * b) >> 16 );	
	
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

void vid_close()
{
	FILE *f;
	setvolume(startvolume);
	
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
	fb.enabled = 0;
}

void vid_settitle(char *title)
{
	SDL_WM_SetCaption(title, title);
}


void vid_begin()
{
	uint8_t *srcbase, *dst_yuv[3];
	uint8_t plane;
	uint8_t width = 160;
	uint8_t height = 144;
	
	if (!emuquit)
	{
		if ((!frameskip) ||  (!useframeskip))
		{
			dst_yuv[0] = screen->pixels;
			dst_yuv[1] = dst_yuv[0] + height * screen->pitch;
			dst_yuv[2] = dst_yuv[1] + height * screen->pitch;
			srcbase = (uint8_t*)fakescreen->pixels;
			for (plane=0; plane<3; plane++) /* The three Y, U and V planes */
			{
				uint32_t y;
				register uint8_t *pal_drm = drm_palette[plane];
				for (y=0; y < height; y++)   /* The number of lines to copy */
				{
					register uint8_t *src = srcbase + (y*fakescreen->w);
					register uint8_t *end_gnuboy = end_gnuboy + width;
					register uint32_t *dst_gnuboy = (uint32_t *)&dst_yuv[plane][width * y];

					__builtin_prefetch(pal_drm, 0, 1 );
					__builtin_prefetch(src, 0, 1 );
					__builtin_prefetch(dst_gnuboy, 1, 0 );

					while (src_gnuboy < end_gnuboy)       /* The actual line data to copy */
					{
						register uint32_t pix_gnuboy;
						pix_gnuboy  = pal_drm[*src_gnuboy++];
						pix_gnuboy |= (pal_drm[*src_gnuboy++])<<8;
						pix_gnuboy |= (pal_drm[*src_gnuboy++])<<16;
						pix_gnuboy |= (pal_drm[*src_gnuboy++])<<24;
						*dst_gnuboy++ = pix_gnuboy;
					}
				}
			}
			SDL_Flip(screen);
		}
		frameskip = !frameskip;
	}
}

void vid_end()
{
	
}
