
#ifndef _SCALER_H
#define _SCALER_H

#include <stdint.h>

extern void bitmap_scale(uint32_t startx, uint32_t starty, uint32_t viswidth, uint32_t visheight, uint32_t newwidth, uint32_t newheight,uint32_t pitchsrc,uint32_t pitchdest, uint16_t* restrict src, uint16_t* restrict dst);
extern void upscale_160x144_to_212x160(uint16_t* restrict src, uint16_t* restrict dst);
extern void upscale_160x144_to_212x144(uint16_t* restrict src, uint16_t* restrict dst);
extern void upscale_160x144_to_240x160(uint16_t* restrict src, uint16_t* restrict dst);
#endif
