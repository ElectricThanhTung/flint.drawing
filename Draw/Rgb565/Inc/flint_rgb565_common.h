
#ifndef __FLINT_RGB565_H
#define __FLINT_RGB565_H

#include "flint_gfx.h"
#include "flint_default_conf.h"

#define PIXEL(_gfx, _x, _y)                                             &((uint16_t *)(_gfx)->data)[(_y) * (_gfx)->w + (_x)]
#define SET_PIXEL(_pixel, _color)                                       do { *(_pixel) = (_color); } while(0)

#define BLEND_PIXEL(_pixel, _alpha, _fg) do {                           \
    uint32_t bg = __builtin_bswap16(*(_pixel));                         \
    bg = (bg | (bg << 16)) & 0x07E0F81F;                                \
    bg += (((_fg) - bg) * (_alpha)) >> 5;                               \
    bg &= 0x07E0F81F;                                                   \
    bg = (bg | (bg >> 16));                                             \
    *(_pixel) = __builtin_bswap16(bg);                                  \
} while(0)

#define SET_H_LINE(_gfx, _color, _x1, _x2, _y) do {                     \
    uint16_t *p = PIXEL(_gfx, _x1, _y);                                 \
    uint16_t *end = PIXEL(_gfx, _x2, _y);                               \
    for(; p <= end; p++) SET_PIXEL(p, _color);                          \
} while(0)

#define BLEND_H_LINE(_gfx, _alpha, _fg, _x1, _x2, _y) do {              \
    uint16_t *p = PIXEL(_gfx, _x1, _y);                                 \
    uint16_t *end = PIXEL(_gfx, _x2, _y);                               \
    for(; p <= end; p++) BLEND_PIXEL(p, _alpha, _fg);                   \
} while(0)

#define SET_V_LINE(_gfx, _color, _y1, _y2, _x) do {                     \
    uint16_t *p = PIXEL(_gfx, _x, _y1);                                 \
    uint16_t *end = PIXEL(_gfx, _x, _y2);                               \
    for(; p <= end; p += (_gfx)->w) SET_PIXEL(p, _color);               \
} while(0)

#define BLEND_V_LINE(_gfx, _alpha, _fg, _y1, _y2, _x) do {              \
    uint16_t *p = PIXEL(_gfx, _x, _y1);                                 \
    uint16_t *end = PIXEL(_gfx, _x, _y2);                               \
    for(; p <= end; p += (_gfx)->w) BLEND_PIXEL(p, _alpha, _fg);        \
} while(0)

void Rgb565_Fill(Gfx *g, uint32_t color, int32_t x1, uint32_t y1, int32_t x2, int32_t y2);

#endif /* __FLINT_RGB565_H */
