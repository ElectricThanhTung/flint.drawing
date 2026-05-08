
#ifndef __FLINT_RGB565_DRAW_RECT_H
#define __FLINT_RGB565_DRAW_RECT_H

#include "flint_rgb565_common.h"

void Rgb565_DrawRect(Gfx *g, uint32_t color, uint32_t thickness, int32_t x, int32_t y, int32_t w, int32_t h);
void Rgb565_FillRect(Gfx *g, uint32_t color, int32_t x, int32_t y, int32_t w, int32_t h);

#endif /* __FLINT_RGB565_DRAW_RECT_H */
