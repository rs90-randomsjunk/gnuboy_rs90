#ifndef FONT_DRAWING_H
#define FONT_DRAWING_H

#include <stdint.h>
#include <string.h>

#define TextWhite 65535
#define TextRed ((255>>3)<<11) + ((0>>2)<<5) + (0>>3)
#define TextBlue ((0>>3)<<11) + ((0>>2)<<5) + (255>>3)

#define HOST_WIDTH_RESOLUTION 240
#define HOST_HEIGHT_RESOLUTION 160

void print_string(const char *s,const uint16_t fg_color, const uint16_t bg_color, int32_t x, int32_t y, uint16_t* restrict buffer);

#endif
