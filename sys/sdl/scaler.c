#include "scaler.h"

/* alekmaul's scaler taken from mame4all */
void bitmap_scale(uint32_t startx, uint32_t starty, uint32_t viswidth, uint32_t visheight, uint32_t newwidth, uint32_t newheight,uint32_t pitchsrc,uint32_t pitchdest, uint16_t* restrict src, uint16_t* restrict dst)
{
    uint32_t W,H,ix,iy,x,y;
    x=startx<<16;
    y=starty<<16;
    W=newwidth;
    H=newheight;
    ix=(viswidth<<16)/W;
    iy=(visheight<<16)/H;

    do 
    {
        uint16_t* restrict buffer_mem=&src[(y>>16)*pitchsrc];
        W=newwidth; x=startx<<16;
        do 
        {
            *dst++=buffer_mem[x>>16];
            x+=ix;
        } while (--W);
        dst+=pitchdest;
        y+=iy;
    } while (--H);
}

#define RMASK 0b1111100000000000
#define GMASK 0b0000011111100000
#define BMASK 0b0000000000011111
#define RSHIFT(X) (((X) & 0xF7DE) >>1) 
//3:2 Alt stretch (sub-pixel scaling)
void upscale_160x144_to_212x144(uint16_t* restrict src, uint16_t* restrict dst){    
    uint16_t* __restrict__ buffer_mem;
    uint16_t* d = dst + 240 * 8;
    const uint16_t ix=3, iy=1;
    
    for (int y = 0; y < 144; y+=iy)
    {
        d+=14;
        int x =0;
        buffer_mem = &src[y * 160];
        for(int w =0; w < 160/3; w++)
        {
            uint16_t r0,r1,g1,b1,r2,g2,b2;
            r0 = buffer_mem[x] & RMASK;
            g1 = buffer_mem[x + 1] & GMASK;
            b1 = buffer_mem[x + 1] & BMASK;
            r1 = buffer_mem[x + 1] & RMASK;
            r2 = buffer_mem[x + 2] & RMASK;
            g2 = buffer_mem[x + 2] & GMASK;
            b2 = buffer_mem[x + 2] & BMASK;
            
            *d++ = buffer_mem[x];
            *d++ = (((r0 + r1) >> 1) & RMASK) | g1 | b1;
            *d++ = r1 | (((g1 + g2) >>1 ) & GMASK) | 
                (((b2 + b1) >> 1 ) & BMASK);
            *d++ = buffer_mem[x + 2];
            x += ix;
        }
        *d = buffer_mem[x];
        d+=14;
    }
}

//upscale 4:3
void upscale_160x144_to_212x160(uint16_t* restrict src, uint16_t* restrict dst){    
    uint16_t* __restrict__ buffer_mem;
    uint16_t* d = dst;
    const uint16_t ix = 3, iy = 9;
    
    for (int y = 0; y < 144; y+=iy)
    {
        d += 14;
        int x = 0;
        buffer_mem = &src[y * 160];
        for(int w = 0; w < 160 / 3; w++)
        {
            uint16_t c[4][9];
            for(int i = 0; i < 9; i++){
                uint16_t r0,r1,g1,b1,r2,g2,b2;
                r0 = buffer_mem[x + i * 160] & RMASK;
                g1 = buffer_mem[x + i * 160 + 1] & GMASK;
                b1 = buffer_mem[x + i * 160 + 1] & BMASK;
                r1 = buffer_mem[x + i * 160 + 1] & RMASK;
                r2 = buffer_mem[x + i * 160 + 2] & RMASK;
                g2 = buffer_mem[x + i * 160 + 2] & GMASK;
                b2 = buffer_mem[x + i * 160 + 2] & BMASK;
            
                c[0][i] = buffer_mem[x + i * 160];
                c[1][i] = (((r0 + r1) >> 1) & RMASK) | g1 | b1;
                c[2][i] = r1 | (((g1 + g2) >>1 ) & GMASK) | 
                (((b2 + b1) >> 1 ) & BMASK);
                c[3][i]= buffer_mem[x + i * 160 + 2];
            }
            for(int i = 0; i < 4 ; i++){
                *d             = c[i][0];
                *(d + 240)     = c[i][1];
                *(d + 240 * 2) = c[i][2];
                *(d + 240 * 3) = c[i][3];
                *(d + 240 * 4) = c[i][4];
                *(d + 240 * 5) = c[i][5];
                *(d + 240 * 6) = c[i][6];
                *(d + 240 * 7) = c[i][7];
                uint16_t r0, g0, b0, r1, g1, b1;
                r0 = c[i][7] & RMASK;
                g0 = c[i][7] & GMASK;
                b0 = c[i][7] & BMASK;
                r1 = c[i][8] & RMASK;
                g1 = c[i][8] & GMASK;
                b1 = c[i][8] & BMASK;
                *(d + 240 * 8) = (((r0>>1) + (r1>>1)) & RMASK) |
                                (((g0 + g1)>>1) & GMASK) |
                                (((b0 + b1)>>1) & BMASK);
                *(d + 240 * 9) = c[i][8];
                d++;
            }
            x += ix;
        }
        for(int i =0 ; i < 10 ; i++){
            *(d + 240 * i) = buffer_mem[x + i * 160];
        }
        
        d += 14;
        d += 240 * 9;
    }
}

//3:2 Full Screen
void upscale_160x144_to_240x160(uint16_t* restrict src, uint16_t* restrict dst){    
    uint16_t* __restrict__ buffer_mem;
    uint16_t* d = dst;
    const uint16_t ix = 2, iy = 9;
    
    for (int y = 0; y < 144; y+=iy)
    {
        int x = 0;
        buffer_mem = &src[y * 160];
        for(int w = 0; w < 160 / 2; w++)
        {
            uint16_t c[3][9];
            for(int i = 0; i < 9; i++){
                uint16_t r0, g0, b0, r1, g1, b1;
                r0 = buffer_mem[x + i * 160]     & RMASK;
                g0 = buffer_mem[x + i * 160]     & GMASK;
                b0 = buffer_mem[x + i * 160]     & BMASK;
                r1 = buffer_mem[x + i * 160 + 1] & RMASK;
                g1 = buffer_mem[x + i * 160 + 1] & GMASK;
                b1 = buffer_mem[x + i * 160 + 1] & BMASK;

                c[0][i] = buffer_mem[x + i * 160];
                c[1][i] = (((r0>>1) + (r0>>2) + (r1>>2)) & RMASK) |
                    (((g0 + g1)>>1) & GMASK) |
                    (((((b0 + b1)>>1) + b1)>>1) & BMASK);
                c[2][i] = buffer_mem[x + i * 160 + 1];
            }
            for(int i = 0; i < 3 ; i++){
                *d             = c[i][0];
                *(d + 240)     = c[i][1];
                *(d + 240 * 2) = c[i][2];
                *(d + 240 * 3) = c[i][3];
                *(d + 240 * 4) = c[i][4];
                *(d + 240 * 5) = c[i][5];
                *(d + 240 * 6) = c[i][6];
                *(d + 240 * 7) = c[i][7];
                uint16_t r0, g0, b0, r1, g1, b1;
                r0 = c[i][7] & RMASK;
                g0 = c[i][7] & GMASK;
                b0 = c[i][7] & BMASK;
                r1 = c[i][8] & RMASK;
                g1 = c[i][8] & GMASK;
                b1 = c[i][8] & BMASK;
                *(d + 240 * 8) = (((r0>>1) + (r1>>1)) & RMASK) |
                                (((g0 + g1)>>1) & GMASK) |
                                (((b0 + b1)>>1) & BMASK);
                *(d + 240 * 9) = c[i][8];
                d++;
            }
            x += ix;
        }
        d += 240 * 9;
    }
}
