
#include <string.h>
#include "flint_default_conf.h"
#include "flint_drawing_common.h"
#include "flint_rgb565_drawing.h"

#if FLINT_API_DRAW_ENABLED

#define FRACTIONAL_BITS             4

#define IN_X_CLIP(_gfx, _x)         ((_x) >= (_gfx)->clipX1 && (_x) <= (_gfx)->clipX2)
#define IN_Y_CLIP(_gfx, _y)         ((_y) >= (_gfx)->clipY1 && (_y) <= (_gfx)->clipY2)
#define IN_CLIP(_gfx, _x, _y)       (IN_X_CLIP((_gfx), (_x)) && IN_Y_CLIP((_gfx), (_y)))

#define PIXEL(_gfx, _x, _y)         &((uint16_t *)(_gfx)->data)[(_y) * (_gfx)->w + (_x)]

#define SET_PIXEL(_pixel, _color) do {                                      \
    *(_pixel) = (_color);                                                   \
} while(0)

#define BLEND_PIXEL(_pixel, _alpha, _fg) do {                               \
    uint32_t bg = __builtin_bswap16(*(_pixel));                             \
    bg = (bg | (bg << 16)) & 0x07E0F81F;                                    \
    bg += (((_fg) - bg) * (_alpha)) >> 5;                                   \
    bg &= 0x07E0F81F;                                                       \
    bg = (bg | (bg >> 16));                                                 \
    *(_pixel) = __builtin_bswap16(bg);                                      \
} while(0)

#define SET_H_LINE(_gfx, _color, _x1, _x2, _y) do {                         \
    uint16_t *p   = PIXEL(_gfx, _x1, _y);                                   \
    uint16_t *end = PIXEL(_gfx, _x2, _y);                                   \
    for(; p <= end; p++) SET_PIXEL(p, _color);                              \
} while(0)

#define BLEND_H_LINE(_gfx, _alpha, _fg, _x1, _x2, _y) do {                  \
    uint16_t *p   = PIXEL(_gfx, _x1, _y);                                   \
    uint16_t *end = PIXEL(_gfx, _x2, _y);                                   \
    for(; p <= end; p++) BLEND_PIXEL(p, _alpha, _fg);                       \
} while(0)

#define SET_V_LINE(_gfx, _color, _y1, _y2, _x) do {                         \
    uint16_t *p = PIXEL(_gfx, _x, _y1);                                     \
    uint16_t *end = PIXEL(_gfx, _x, _y2);                                   \
    for(; p <= end; p += (_gfx)->w) SET_PIXEL(p, _color);                   \
} while(0)

#define BLEND_V_LINE(_gfx, _alpha, _fg, _y1, _y2, _x) do {                  \
    uint16_t *p = PIXEL(_gfx, _x, _y1);                                     \
    uint16_t *end = PIXEL(_gfx, _x, _y2);                                   \
    for(; p <= end; p += (_gfx)->w) BLEND_PIXEL(p, _alpha, _fg);            \
} while(0)

static void Rgb565_FillRect(FGfx *g, uint32_t color, int32_t x1, uint32_t y1, int32_t x2, int32_t y2) {
    uint8_t alpha = color >> 27;
    if(alpha == 0x1F) {
        uint16_t c = color;
        if((x2 - x1) == 1)
            SET_V_LINE(g, color, y1, y2, x1);
        else for(uint32_t y = y1; y <= y2; y++)
            SET_H_LINE(g, color, x1, x2, y);
    }
    else {
        uint32_t fg = __builtin_bswap16(color);
        fg = (fg | (fg << 16)) & 0x07E0F81F;
        if((x2 - x1) == 1)
            BLEND_V_LINE(g, alpha, fg, y1, y2, x1);
        else for(uint32_t y = y1; y <= y2; y++)
            BLEND_H_LINE(g, alpha, fg, x1, x2, y);
    }
}

static void Rgb565_DrawHLine(FGfx *g, uint32_t color, uint32_t width, int32_t x1, int32_t x2, int32_t y) {
    if(x1 > x2) F_SWAP(x1, x2);
    int32_t half = (width - 1) >> 1;
    int32_t xmin = F_MAX(g->clipX1, x1);
    int32_t xmax = F_MIN(g->clipX2, x2);
    int32_t ymin = F_MAX(g->clipY1, y - half);
    int32_t ymax = F_MIN(g->clipY2, y + half + 1);

    if((width & 0x01) == 0) {
        int32_t y1 = y - half - 1;
        int32_t y2 = y + half + 1;

        uint32_t a1 = color >> 28;
        uint32_t a2 = color >> 29;
        uint32_t fg = __builtin_bswap16(color);
        fg = (fg | (fg << 16)) & 0x07E0F81F;

        if(IN_X_CLIP(g, x1)) {
            BLEND_V_LINE(g, a1, fg, ymin, ymax, x1);
            if(IN_Y_CLIP(g, y1)) BLEND_PIXEL(&((uint16_t *)g->data)[y1 * g->w + x1], a2, fg);
            if(IN_Y_CLIP(g, y2)) BLEND_PIXEL(&((uint16_t *)g->data)[y2 * g->w + x1], a2, fg);
        }

        x2--;
        if(IN_X_CLIP(g, x2)) {
            BLEND_V_LINE(g, a1, fg, ymin, ymax, x2);
            if(IN_Y_CLIP(g, y1)) BLEND_PIXEL(&((uint16_t *)g->data)[y1 * g->w + x2], a2, fg);
            if(IN_Y_CLIP(g, y2)) BLEND_PIXEL(&((uint16_t *)g->data)[y2 * g->w + x2], a2, fg);
        }
        xmin++;
        xmax--;
        if(y1 >= g->clipY1) BLEND_H_LINE(g, a1, fg, xmin, xmax, y1);
        if(y2 <= g->clipY2) BLEND_H_LINE(g, a1, fg, xmin, xmax, y2);
    }
    Rgb565_FillRect(g, color, xmin, ymin, xmax, ymax);
}

static void Rgb565_DrawVLine(FGfx *g, uint32_t color, uint32_t width, int32_t y1, int32_t y2, int32_t x) {
    if(y1 > y2) F_SWAP(y1, y2);
    int32_t half = (width - 1) >> 1;
    int32_t ymin = F_MAX(g->clipY1, y1);
    int32_t ymax = F_MIN(g->clipY2, y2);
    int32_t xmin = F_MAX(g->clipX1, x - half);
    int32_t xmax = F_MIN(g->clipX2, x + half + 1);

    if((width & 0x01) == 0) {
        int32_t x1 = x - half - 1;
        int32_t x2 = x + half + 1;

        uint32_t a1 = color >> 28;
        uint32_t a2 = color >> 29;
        uint32_t fg = __builtin_bswap16(color);
        fg = (fg | (fg << 16)) & 0x07E0F81F;

        if(IN_Y_CLIP(g, y1)) {
            BLEND_H_LINE(g, a1, fg, xmin, xmax, y1);
            if(IN_X_CLIP(g, x1)) BLEND_PIXEL(&((uint16_t *)g->data)[y1 * g->w + x1], a2, fg);
            if(IN_X_CLIP(g, x2)) BLEND_PIXEL(&((uint16_t *)g->data)[y1 * g->w + x2], a2, fg);
        }

        y2--;
        if(IN_Y_CLIP(g, y2)) {
            BLEND_H_LINE(g, a1, fg, xmin, xmax, y2);
            if(IN_X_CLIP(g, x1)) BLEND_PIXEL(&((uint16_t *)g->data)[y2 * g->w + x1], a2, fg);
            if(IN_X_CLIP(g, x2)) BLEND_PIXEL(&((uint16_t *)g->data)[y2 * g->w + x2], a2, fg);
        }
        ymin++;
        ymax--;
        if(x1 >= g->clipX1) BLEND_V_LINE(g, a1, fg, ymin, ymax, x1);
        if(x2 <= g->clipX2) BLEND_V_LINE(g, a1, fg, ymin, ymax, x2);
    }
    Rgb565_FillRect(g, color, xmin, ymin, xmax, ymax);
}

static void Rgb565_DrawSkewLine(FGfx *g, uint32_t color, uint32_t width, int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
    int32_t dx = F_ABS(x2 - x1);
    int32_t dy = F_ABS(y2 - y1);
    uint8_t alpha = color >> 27;
    uint32_t fg = __builtin_bswap16(color);
    fg = (fg | (fg << 16)) & 0x07E0F81F;

    if(width == 1) {
        int32_t err = 0;
        if(dx > dy) {
            int8_t ystep = (y1 < y2) ? 1 : -1;
            int32_t y = y1;
            if(x1 > x2) {
                F_SWAP(x1, x2);
                F_SWAP(y1, y2);
            }
            uint16_t *p = PIXEL(g, 0, y);
            for(int32_t x = x1; x <= x2; x++) {
                if(IN_X_CLIP(g, x)) {
                    int32_t a = err * alpha / dx;
                    if(IN_Y_CLIP(g, y)) BLEND_PIXEL(&p[x], alpha - a, fg);
                    if(IN_Y_CLIP(g, y - ystep)) BLEND_PIXEL(&p[x - ystep * g->w], a, fg);
                }
                err -= dy;
                if(err < 0) {
                    y += ystep;
                    err += dx;
                    p = PIXEL(g, 0, y);
                }
            }
        }
        else {
            int8_t xstep = (x1 < x2) ? 1 : -1;
            int32_t x = x1;
            if(y1 > y2) {
                F_SWAP(x1, x2);
                F_SWAP(y1, y2);
            }
            uint16_t *p = PIXEL(g, x, 0);
            for(int32_t y = y1; y <= y2; y++) {
                if(IN_Y_CLIP(g, y)) {
                    int32_t a = err * alpha / dy;
                    if(IN_X_CLIP(g, x)) BLEND_PIXEL(&p[y * g->w], alpha - a, fg);
                    if(IN_X_CLIP(g, x - xstep)) BLEND_PIXEL(&p[y * g->w - xstep], a, fg);
                }
                err -= dx;
                if(err < 0) {
                    x += xstep;
                    err += dy;
                    p = PIXEL(g, x, 0);
                }
            }
        }
    }
    else {
        static constexpr uint16_t fmax = (1 << FRACTIONAL_BITS);
        static constexpr uint16_t fmask = fmax - 1;
        static const uint8_t w[] = {
            255, 251, 247, 244, 240, 237, 234, 231, 228, 225, 222, 220, 217, 215, 212, 210,
            208, 206, 203, 201, 199, 198, 196, 194, 192, 190, 189, 187, 186, 184, 182, 181,
            180, 178, 177, 176, 174, 173, 172, 170, 169, 168, 167, 166, 165, 164, 163, 162,
            161, 160, 159, 158, 157, 156, 155, 154, 153, 152, 151, 150, 150, 149, 148, 147,
            255
        };
        int32_t wx, wy;
        if(dx > dy) {
            wy = (width * w[(dy << 6) / dx] + 127) >> 8;
            wx = (wy * dy + dx / 2) / dx;
        }
        else {
            wx = (width * w[(dx << 6) / dy] + 127) >> 8;
            wy = (wx * dx + dy / 2) / dy;
        }
        if(x1 > x2) {
            F_SWAP(x1, x2);
            F_SWAP(y1, y2);
        }

        int32_t wy0 = wy >> 1;
        int32_t wy1 = wy0 + (wy & 0x1);  
        int32_t wx0 = wx >> 1;
        int32_t wx1 = wx0 + (wx & 0x01);

        FPoint p1 = (y1 < y2) ? (FPoint){x1 - wx0, y1 + wy1} : (FPoint){x1 - wx0, y1 - wy0};
        FPoint p2 = (y1 < y2) ? (FPoint){x1 + wx1, y1 - wy0} : (FPoint){x2 - wx0, y2 - wy0};
        FPoint p3 = (y1 < y2) ? (FPoint){x2 + wx1, y2 - wy0} : (FPoint){x2 + wx1, y2 + wy1};
        FPoint p4 = (y1 < y2) ? (FPoint){x2 - wx0, y2 + wy1} : (FPoint){x1 + wx1, y1 + wy1};

        int32_t ymin = F_MAX(g->clipY1, p2.y);
        int32_t ymax = F_MIN(g->clipY2, F_MIN(p1.y, p3.y));
        int32_t y = ymin;

        int16_t a0, a;

        int32_t dx1 = p2.x - p1.x;
        int32_t dy1 = p1.y - p2.y;
        int32_t dx2 = p3.x - p2.x;
        int32_t dy2 = p3.y - p2.y;

        for(; y <= ymax; y++) {
            int32_t x1 = (p1.y - y) * dx1;
            int32_t x2 = (y - p2.y) * dx2;
            int32_t fx1 = ((x1 << FRACTIONAL_BITS) / dy1) & fmask;
            int32_t fx2 = ((x2 << FRACTIONAL_BITS) / dy2) & fmask;
            x1 = p1.x + x1 / dy1;
            x2 = p2.x + x2 / dy2;
            a = a0 = dx1 < (dy1 << 1) ? ((fmax - fx1) * alpha / fmax) : alpha;
            for(uint32_t i = x1; a > 0 && i >= p1.x;) {
                if(IN_X_CLIP(g, i)) BLEND_PIXEL(PIXEL(g, i, y), a, fg);
                a = a0 - ((x1 - (--i)) * alpha) * dy1 / dx1;
            }
            x1++;
            a = a0 = dx2 < (dy2 << 1) ? (fx2 * alpha / fmax) : alpha;
            for(uint32_t i = x2 + 1; a > 0 && i <= p3.x;) {
                if(IN_X_CLIP(g, i)) BLEND_PIXEL(PIXEL(g, i, y), a, fg);
                a = a0 - (((++i) - x2 - 1) * alpha) * dy2 / dx2;
            }
            if(alpha == 0x1F) SET_H_LINE(g, color, F_MAX(g->clipX1, x1), F_MIN(g->clipX2, x2), y);
            else BLEND_H_LINE(g, alpha, fg, F_MAX(g->clipX1, x1), F_MIN(g->clipX2, x2), y);
        }
        ymax = F_MIN(g->clipY2, F_MAX(p1.y, p3.y));
        if(p1.y < p3.y) for(; y < ymax; y++) {
            int32_t x1 = (y - p1.y) * dx2;
            int32_t x2 = (y - p2.y) * dx2;
            int32_t fx1 = ((x1 << FRACTIONAL_BITS) / dy2) & fmask;
            int32_t fx2 = ((x2 << FRACTIONAL_BITS) / dy2) & fmask;
            x1 = p1.x + x1 / dy2;
            x2 = p2.x + x2 / dy2;
            a = a0 = dx2 < (dy2 << 1) ? ((fmax - fx1) * alpha / fmax) : alpha;
            for(uint32_t i = x1; a > 0 && i >= p1.x;) {
                if(IN_X_CLIP(g, i)) BLEND_PIXEL(PIXEL(g, i, y), a, fg);
                a = a0 - ((x1 - (--i)) * alpha) * dy2 / dx2;
            }
            x1++;
            a = a0 = dx2 < (dy2 << 1) ? (fx2 * alpha / fmax) : alpha;
            for(uint32_t i = x2 + 1; a > 0 && i <= p3.x;) {
                if(IN_X_CLIP(g, i)) BLEND_PIXEL(PIXEL(g, i, y), a, fg);
                a = a0 - (((++i) - x2 - 1) * alpha) * dy2 / dx2;
            }
            if(alpha == 0x1F) SET_H_LINE(g, color, F_MAX(g->clipX1, x1), F_MIN(g->clipX2, x2), y);
            else BLEND_H_LINE(g, alpha, fg, F_MAX(g->clipX1, x1), F_MIN(g->clipX2, x2), y);
        }
        else for(; y < ymax; y++) {
            int32_t x1 = (p1.y - y) * dx1;
            int32_t x2 = (p4.y - y) * dx1;
            int32_t fx1 = ((x1 << FRACTIONAL_BITS) / dy1) & fmask;
            int32_t fx2 = ((x2 << FRACTIONAL_BITS) / dy1) & fmask;
            x1 = p1.x + x1 / dy1;
            x2 = p4.x + x2 / dy1;
            a = a0 = dx1 < (dy1 << 1) ? ((fmax - fx1) * alpha / fmax) : alpha;
            for(uint32_t i = x1; a > 0 && i >= p1.x;) {
                if(IN_X_CLIP(g, i)) BLEND_PIXEL(PIXEL(g, i, y), a, fg);
                a = a0 - ((x1 - (--i)) * alpha) * dy1 / dx1;
            }
            x1++;
            a = a0 = dx1 < (dy1 << 1) ? (fx2 * alpha / fmax) : alpha;
            for(uint32_t i = x2 + 1; a > 0 && i <= p3.x;) {
                if(IN_X_CLIP(g, i)) BLEND_PIXEL(PIXEL(g, i, y), a, fg);
                a = a0 - (((++i) - x2 - 1) * alpha) * dy1 / dx1;
            }
            if(alpha == 0x1F) SET_H_LINE(g, color, F_MAX(g->clipX1, x1), F_MIN(g->clipX2, x2), y);
            else BLEND_H_LINE(g, alpha, fg, F_MAX(g->clipX1, x1), F_MIN(g->clipX2, x2), y);
        }
        ymax = F_MIN(g->clipY2, p4.y);
        for(; y <= ymax; y++) {
            int32_t x1 = (y - p1.y) * dx2;
            int32_t x2 = (p4.y - y) * dx1;
            int32_t fx1 = ((x1 << FRACTIONAL_BITS) / dy2) & fmask;
            int32_t fx2 = ((x2 << FRACTIONAL_BITS) / dy1) & fmask;
            x1 = p1.x + x1 / dy2;
            x2 = p4.x + x2 / dy1;
            a = a0 = dx2 < (dy2 << 1) ? ((fmax - fx1) * alpha / fmax) : alpha;
            for(uint32_t i = x1; a > 0 && i >= p1.x;) {
                if(IN_X_CLIP(g, i)) BLEND_PIXEL(PIXEL(g, i, y), a, fg);
                a = a0 - ((x1 - (--i)) * alpha) * dy2 / dx2;
            }
            x1++;
            a = a0 = dx1 < (dy1 << 1) ? (fx2 * alpha / fmax) : alpha;
            for(uint32_t i = x2 + 1; a > 0 && i <= p3.x;) {
                if(IN_X_CLIP(g, i)) BLEND_PIXEL(PIXEL(g, i, y), a, fg);
                a = a0 - (((++i) - x2 - 1) * alpha) * dy1 / dx1;
            }
            if(alpha == 0x1F) SET_H_LINE(g, color, F_MAX(g->clipX1, x1), F_MIN(g->clipX2, x2), y);
            else BLEND_H_LINE(g, alpha, fg, F_MAX(g->clipX1, x1), F_MIN(g->clipX2, x2), y);
        }

        SET_PIXEL(PIXEL(g, p1.x, p1.y), 0xF8);
        SET_PIXEL(PIXEL(g, p2.x, p2.y), 0xF8);
        SET_PIXEL(PIXEL(g, p3.x, p3.y), 0xF8);
        SET_PIXEL(PIXEL(g, p4.x, p4.y), 0xF8);
    }
}

void Rgb565_DrawLine(FGfx *g, uint32_t color, uint32_t width, int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
    if(x1 == x2 && y1 == y2)
        return;
    else if(y1 == y2)
        Rgb565_DrawHLine(g, color, width, x1, x2, y1);
    else if(x1 == x2)
        Rgb565_DrawVLine(g, color, width, y1, y2, x1);
    else
        Rgb565_DrawSkewLine(g, color, width, x1, y1, x2, y2);
}

#endif /* FLINT_API_DRAW_ENABLED */
