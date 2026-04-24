
#ifndef __FLINT_RGB565_DRAW_LINE_H
#define __FLINT_RGB565_DRAW_LINE_H

#include "flint_rgb565_common.h"

#if FLINT_API_DRAW_ENABLED

void Rgb565_DrawLine(FGfx *g, uint32_t color, uint32_t thickness, int32_t x1, int32_t y1, int32_t x2, int32_t y2);

#endif /* FLINT_API_DRAW_ENABLED */

#endif /* __FLINT_RGB565_DRAW_LINE_H */
