
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

//sub-pixel scaling
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
            uint16_t r0,r1,g1,b1,b2;
            r0 = buffer_mem[x]     & 0b1111100000000000;
            g1 = buffer_mem[x + 1] & 0b0000011111100000;
            b1 = buffer_mem[x + 1] & 0b0000000000011111;
            r1 = buffer_mem[x + 1] & 0b1111100000000000;
            b2 = buffer_mem[x + 2] & 0b0000000000011111;
            
            *d++ = buffer_mem[x];
            *d++ = r0 | g1 | b1;
            *d++ = r1 | g1 | b2;
            *d++ = buffer_mem[x + 2];
            x += ix;
        }
        *d = buffer_mem[x];
        d+=14;
    }
}

//upscale 4:3(blurry)
#define RSHIFT(X) (((X) & 0xF7DE) >>1) 
void upscale_160x144_to_212x160(uint16_t* restrict src, uint16_t* restrict dst){    
    uint16_t* __restrict__ buffer_mem;
    uint16_t* d = dst;
    const uint16_t ix=3, iy=9;
    
    for (int y = 0; y < 144; y+=iy)
    {
        d+=14;
        int x =0;
        buffer_mem = &src[y * 160];
        for(int w =0; w < 160/3; w++)
        {
            uint16_t a[9],b[9],c[9];
            for(int i =0; i < 9; i++){
                a[i]=RSHIFT(buffer_mem[x + 160 * i]);
                b[i]=RSHIFT(buffer_mem[x + 160 * i + 1]);
                c[i]=RSHIFT(buffer_mem[x + 160 * i + 2]);
            }
            //A0~A9
            *d         = a[0]<<1;
            *(d+240)   = a[1] + RSHIFT(a[1] + RSHIFT(a[1]+ a[0]));
            *(d+240*2) = a[2] + RSHIFT(a[1] + a[2]);
            *(d+240*3) = a[3] + RSHIFT(a[2] + RSHIFT(a[2] + a[3]));
            *(d+240*4) = a[4] + RSHIFT(a[3] + RSHIFT(a[3] + RSHIFT(a[3] + a[4])));
            *(d+240*5) = a[4] + RSHIFT(a[5] + RSHIFT(a[5] + RSHIFT(a[5] + a[4])));
            *(d+240*6) = a[5] + RSHIFT(a[6] + RSHIFT(a[5] + a[6]));
            *(d+240*7) = a[6] + RSHIFT(a[6] + a[7]);
            *(d+240*8) = a[7] + RSHIFT(a[7] + RSHIFT(a[7] + a[8]));
            *(d+240*9) = a[8]<<1;
            d++;
            
        //B9~B9
            *d         = b[0] +RSHIFT(a[0] + RSHIFT(a[0] +  b[0]));
            *(d+240)   = b[1] + RSHIFT(a[1] + RSHIFT(b[1] + RSHIFT(a[1] + b[0])));
            *(d+240*2) = b[2] + RSHIFT(a[2] + RSHIFT(b[1] + RSHIFT(b[2] + a[1])));
            *(d+240*3) = RSHIFT(a[3] + b[2]) + RSHIFT(b[3] + RSHIFT(a[2] + b[3]));
            *(d+240*4) = RSHIFT(b[3] + b[4]) + RSHIFT(RSHIFT(a[3] + a[4]) + RSHIFT(b[4] + RSHIFT(a[4] + b[3])));
            *(d+240*5) = RSHIFT(b[4] + b[5]) +RSHIFT(RSHIFT(a[4] + a[5]) + RSHIFT(b[4] + RSHIFT(b[5] + a[4])));
            *(d+240*6) = RSHIFT(a[5] + b[5]) + RSHIFT(b[6] + RSHIFT(a[6] + b[5]));
            *(d+240*7) = b[6] + RSHIFT(a[6]+ RSHIFT(b[7] + RSHIFT(a[7] + b[6])));
            *(d+240*8) = b[7] + RSHIFT(a[7] + RSHIFT(b[7] + RSHIFT(b[8] + a[7])));
            *(d+240*9) = b[8] + RSHIFT(a[8] + RSHIFT(a[8] + b[8]));
            d++;
        //C0~C9
            *d         = b[0] + RSHIFT(c[0] + RSHIFT(c[0] + b[0]));
            *(d+240)   = b[1] + RSHIFT(c[1] + RSHIFT(b[1] + RSHIFT(c[1] + b[0])));
            *(d+240*2) = b[2] + RSHIFT(c[2] + RSHIFT(b[1] + RSHIFT(b[2] + c[1])));
            *(d+240*3) = RSHIFT(c[3] + b[2]) + RSHIFT(b[3] + RSHIFT(c[2] + b[3]));
            *(d+240*4) = RSHIFT(b[3] + b[4]) + RSHIFT(RSHIFT(b[4] + c[3]) + RSHIFT(c[4] + RSHIFT(c[4] + b[3])));
            *(d+240*5) = RSHIFT(b[4] + b[5]) + RSHIFT(RSHIFT(b[4] + c[4]) + RSHIFT(c[5] + RSHIFT(b[5] + c[4])));
            *(d+240*6) = RSHIFT(b[5] + b[6]) + RSHIFT(c[5] + RSHIFT(c[6] + b[5]));
            *(d+240*7) = b[6] + RSHIFT(c[6] + RSHIFT(b[7] + RSHIFT(c[7] + b[6])));
            *(d+240*8) = b[7] + RSHIFT(c[7] + RSHIFT(b[7] + RSHIFT(c[7] + b[8])));
            *(d+240*9) = b[8] + RSHIFT(c[8] + RSHIFT(c[8] + b[8]));
            d++;
       //D0~D9
            *d         = c[0]<<1;
            *(d+240)   = c[1] + RSHIFT(c[1] + RSHIFT(c[1] + c[0]));
            *(d+240*2) = c[2] + RSHIFT(c[1] + c[2]);
            *(d+240*3) = c[3] + RSHIFT(c[2] + RSHIFT(c[2] + c[3]));
            *(d+240*4) = c[4] + RSHIFT(c[3] + RSHIFT(c[3] + RSHIFT(c[3] + c[4])));
            *(d+240*5) = c[4] + RSHIFT(c[5] + RSHIFT(c[5] + RSHIFT(c[5] + c[4])));
            *(d+240*6) = c[5] + RSHIFT(c[6] + RSHIFT(c[5] + c[6]));
            *(d+240*7) = c[6] + RSHIFT(c[6] + c[7]);
            *(d+240*8) = c[7] + RSHIFT(c[7] + RSHIFT(c[7] + c[8]));
            *(d+240*9) = c[8]<<1;
            d++;
            x += ix;
        }
        //last one line
        uint16_t a[9];
        for(int i =0; i < 9; i++){
            a[i]=RSHIFT(buffer_mem[x + 160 * i]);
        }
        //A0~A9
        *d         = a[0]<<1;
        *(d+240)   = a[1] + RSHIFT(a[1] + RSHIFT(a[1]+ a[0]));
        *(d+240*2) = a[2] + RSHIFT(a[1] + a[2]);
        *(d+240*3) = a[3] + RSHIFT(a[2] + RSHIFT(a[2] + a[3]));
        *(d+240*4) = a[4] + RSHIFT(a[3] + RSHIFT(a[3] + RSHIFT(a[3] + a[4])));
        *(d+240*5) = a[4] + RSHIFT(a[5] + RSHIFT(a[5] + RSHIFT(a[5] + a[4])));
        *(d+240*6) = a[5] + RSHIFT(a[6] + RSHIFT(a[5] + a[6]));
        *(d+240*7) = a[6] + RSHIFT(a[6] + a[7]);
        *(d+240*8) = a[7] + RSHIFT(a[7] + RSHIFT(a[7] + a[8]));
        *(d+240*9) = a[8]<<1;

        d+=14 + 240 * 9;
    }
}

void upscale_160x144_to_240x160(uint16_t* restrict src, uint16_t* restrict dst){    
    uint16_t* __restrict__ buffer_mem;
    uint16_t* d = dst;
    const uint16_t ix=2, iy=9;
    
    for (int y = 0; y < 144; y+=iy)
    {
        int x =0;
        buffer_mem = &src[y * 160];
        for(int w =0; w < 160 / 2; w++)
        {
            uint16_t c[3][10];
            for(int i=0; i<10; i++){
                uint16_t r0,r1,g0,g1,b1,b2;
                r0 = buffer_mem[x + i * 160]     & 0b1111100000000000;
                g0 = buffer_mem[x + i * 160]     & 0b0000011111000000;
                g1 = buffer_mem[x + i * 160 + 1] & 0b0000011111000000;
                b1 = buffer_mem[x + i * 160 + 1] & 0b0000000000011111;

                c[0][i] = buffer_mem[x + i * 160];
                c[1][i] = r0 | ((g0 + g1)>>1) | b1;
                c[2][i] = buffer_mem[x + i * 160 + 1];
            }
            for(int i = 0; i<3 ; i++){
                *d        = c[i][0];
                *(d +240) = c[i][1];
                *(d +240 * 2) = c[i][2];
                *(d +240 * 3) = c[i][3];
                *(d +240 * 4) = c[i][4];
                *(d +240 * 5) = c[i][5];
                *(d +240 * 6) = c[i][6];
                *(d +240 * 7) = c[i][7];
                *(d +240 * 8) = c[i][8];
                *(d +240 * 9) = c[i][8];
                  d++;
            }
            x += ix;
        }
        d += 240 * 9;
    }
}