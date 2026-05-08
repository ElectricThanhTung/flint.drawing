
#include <string.h>
#include "flint_default_conf.h"
#include "flint_rgb565_common.h"
#include "flint_rgb565_draw_line.h"

typedef struct {
    int32_t x;
    int32_t y;
} Point;

static void Rgb565_DrawHLine(Gfx *g, uint32_t color, uint32_t thickness, int32_t x1, int32_t x2, int32_t y) {
    if(x1 > x2) F_SWAP(x1, x2);
    int32_t half = (thickness - 1) >> 1;
    int32_t xmin = F_X1(g, x1);
    int32_t xmax = F_X2(g, x2);
    int32_t ymin = F_Y1(g, y - half);
    int32_t ymax = F_Y2(g, y + half);

    if((thickness & 0x01) == 0) {
        int32_t y1 = y - half - 1;
        int32_t y2 = y + half + 1;

        uint32_t a1 = color >> 28;
        uint32_t a2 = color >> 29;
        uint32_t fg = __builtin_bswap16(color);
        fg = (fg | (fg << 16)) & 0x07E0F81F;

        if(IN_X_CLIP(g, x1)) {
            BLEND_V_LINE(g, a1, fg, ymin, ymax, x1);
            if(IN_Y_CLIP(g, y1)) BLEND_PIXEL(PIXEL(g, x1, y1), a2, fg);
            if(IN_Y_CLIP(g, y2)) BLEND_PIXEL(PIXEL(g, x1, y2), a2, fg);
        }

        if(IN_X_CLIP(g, x2)) {
            BLEND_V_LINE(g, a1, fg, ymin, ymax, x2);
            if(IN_Y_CLIP(g, y1)) BLEND_PIXEL(PIXEL(g, x2, y1), a2, fg);
            if(IN_Y_CLIP(g, y2)) BLEND_PIXEL(PIXEL(g, x2, y2), a2, fg);
        }
        xmin++;
        xmax--;
        if(y1 >= g->clipY1) BLEND_H_LINE(g, a1, fg, xmin, xmax, y1);
        if(y2 <= g->clipY2) BLEND_H_LINE(g, a1, fg, xmin, xmax, y2);
    }
    Rgb565_Fill(g, color, xmin, ymin, xmax, ymax);
}

static void Rgb565_DrawVLine(Gfx *g, uint32_t color, uint32_t thickness, int32_t y1, int32_t y2, int32_t x) {
    if(y1 > y2) F_SWAP(y1, y2);
    int32_t half = (thickness - 1) >> 1;
    int32_t ymin = F_Y1(g, y1);
    int32_t ymax = F_Y2(g, y2);
    int32_t xmin = F_X1(g, x - half);
    int32_t xmax = F_X2(g, x + half);

    if((thickness & 0x01) == 0) {
        int32_t x1 = x - half - 1;
        int32_t x2 = x + half + 1;

        uint32_t a1 = color >> 28;
        uint32_t a2 = color >> 29;
        uint32_t fg = __builtin_bswap16(color);
        fg = (fg | (fg << 16)) & 0x07E0F81F;

        if(IN_Y_CLIP(g, y1)) {
            BLEND_H_LINE(g, a1, fg, xmin, xmax, y1);
            if(IN_X_CLIP(g, x1)) BLEND_PIXEL(PIXEL(g, x1, y1), a2, fg);
            if(IN_X_CLIP(g, x2)) BLEND_PIXEL(PIXEL(g, x2, y1), a2, fg);
        }

        if(IN_Y_CLIP(g, y2)) {
            BLEND_H_LINE(g, a1, fg, xmin, xmax, y2);
            if(IN_X_CLIP(g, x1)) BLEND_PIXEL(PIXEL(g, x1, y2), a2, fg);
            if(IN_X_CLIP(g, x2)) BLEND_PIXEL(PIXEL(g, x2, y2), a2, fg);
        }
        ymin++;
        ymax--;
        if(x1 >= g->clipX1) BLEND_V_LINE(g, a1, fg, ymin, ymax, x1);
        if(x2 <= g->clipX2) BLEND_V_LINE(g, a1, fg, ymin, ymax, x2);
    }
    Rgb565_Fill(g, color, xmin, ymin, xmax, ymax);
}

static void CalcRectPoints(uint32_t width, int32_t x1, int32_t y1, int32_t x2, int32_t y2, Point *points) {
    static const uint8_t wratio[] = {
        255, 251, 247, 244, 240, 237, 234, 231, 228, 225, 222, 220, 217, 215, 212, 210,
        208, 206, 203, 201, 199, 198, 196, 194, 192, 190, 189, 187, 186, 184, 182, 181,
        180, 178, 177, 176, 174, 173, 172, 170, 169, 168, 167, 166, 165, 164, 163, 162,
        161, 160, 159, 158, 157, 156, 155, 154, 153, 152, 151, 150, 150, 149, 148, 147,
        255
    };
    int32_t dx = F_ABS(x2 - x1);
    int32_t dy = F_ABS(y2 - y1);
    int32_t wx, wy;

    if(dx > dy) {
        wy = (width * wratio[(dy << 6) / dx] + 127) >> 8;
        wx = (wy * dy + dx / 2) / dx;
    }
    else {
        wx = (width * wratio[(dx << 6) / dy] + 127) >> 8;
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

    if(y1 < y2) {
        points[0] = {x1 - wx0, y1 + wy1};
        points[1] = {x1 + wx1, y1 - wy0};
        points[2] = {x2 + wx1, y2 - wy0};
        points[3] = {x2 - wx0, y2 + wy1};
    }
    else {
        points[0] = {x1 - wx0, y1 - wy0};
        points[1] = {x2 - wx0, y2 - wy0};
        points[2] = {x2 + wx1, y2 + wy1};
        points[3] = {x1 + wx1, y1 + wy1};
    }
}

static void Rgb565_DrawSkewLine(Gfx *g, uint32_t color, uint32_t thickness, int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
    uint8_t alpha = color >> 27;
    uint32_t fg = __builtin_bswap16(color);
    fg = (fg | (fg << 16)) & 0x07E0F81F;

    if(thickness == 1) {
        int32_t err = 0;
        int32_t dx = F_ABS(x2 - x1);
        int32_t dy = F_ABS(y2 - y1);
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
        static constexpr uint16_t fpmax = (1 << FRACTIONAL_BITS);
        static constexpr uint16_t fpmask = fpmax - 1;
        Point p[4];
        CalcRectPoints(thickness, x1, y1, x2, y2, p);

        int32_t ymin = F_Y1(g, p[1].y);
        int32_t ymax = F_Y2(g, F_MIN(p[0].y, p[2].y));
        int32_t y = ymin;

        int32_t dx1 = p[1].x - p[0].x;
        int32_t dy1 = p[0].y - p[1].y;
        int32_t dx2 = p[2].x - p[1].x;
        int32_t dy2 = p[2].y - p[1].y;
        uint8_t astep1 = (alpha * dy1 + (dx1 >> 1)) / dx1;
        uint8_t astep2 = (alpha * dy2 + (dx2 >> 1)) / dx2;

        for(; y <= ymax; y++) {
            int32_t x1 = (p[0].y - y) * dx1;
            int32_t fx1 = fpmax - (((x1 << FRACTIONAL_BITS) / dy1) & fpmask);
            x1 = p[0].x + x1 / dy1;

            int32_t x2 = (y - p[1].y) * dx2 + dy2 - 1;
            int32_t fx2 = ((x2 << FRACTIONAL_BITS) / dy2) & fpmask;
            x2 = p[1].x + x2 / dy2;

            int16_t a = (dx1 < dy1) ? (fx1 * alpha / fpmax) : (alpha - astep1 + fx1 * dy1 / dx1 * alpha / fpmax);
            for(uint32_t i = x1++; a > 0 && i >= p[0].x; i--) {
                if(IN_X_CLIP(g, i)) BLEND_PIXEL(PIXEL(g, i, y), a, fg);
                a -= astep1;
            }

            a = (dx2 < dy2) ? (fx2 * alpha / fpmax) : (alpha - astep2 + fx2 * dy2 / dx2 * alpha / fpmax);
            for(uint32_t i = x2--; a > 0 && i <= p[2].x; i++) {
                if(IN_X_CLIP(g, i)) BLEND_PIXEL(PIXEL(g, i, y), a, fg);
                a -= astep2;
            }
            if(alpha == 0x1F) SET_H_LINE(g, color, F_X1(g, x1), F_X2(g, x2), y);
            else BLEND_H_LINE(g, alpha, fg, F_X1(g, x1), F_X2(g, x2), y);
        }
        ymax = F_Y2(g, F_MAX(p[0].y, p[2].y));
        if(p[0].y < p[2].y) for(; y < ymax; y++) {
            int32_t x1 = (y - p[0].y) * dx2;
            int32_t fx1 = fpmax - (((x1 << FRACTIONAL_BITS) / dy2) & fpmask);
            x1 = p[0].x + x1 / dy2;

            int32_t x2 = (y - p[1].y) * dx2 + dy2 - 1;
            int32_t fx2 = ((x2 << FRACTIONAL_BITS) / dy2) & fpmask;
            x2 = p[1].x + x2 / dy2;

            int16_t a = (dx2 < dy2) ? (fx1 * alpha / fpmax) : (alpha - astep2 + fx1 * dy2 / dx2 * alpha / fpmax);
            for(uint32_t i = x1++; a > 0 && i >= p[0].x; i--) {
                if(IN_X_CLIP(g, i)) BLEND_PIXEL(PIXEL(g, i, y), a, fg);
                a -= astep2;
            }

            a = (dx2 < dy2) ? (fx2 * alpha / fpmax) : (alpha - astep2 + fx2 * dy2 / dx2 * alpha / fpmax);
            for(uint32_t i = x2--; a > 0 && i <= p[2].x; i++) {
                if(IN_X_CLIP(g, i)) BLEND_PIXEL(PIXEL(g, i, y), a, fg);
                a -= astep2;
            }

            if(alpha == 0x1F) SET_H_LINE(g, color, F_X1(g, x1), F_X2(g, x2), y);
            else BLEND_H_LINE(g, alpha, fg, F_X1(g, x1), F_X2(g, x2), y);
        }
        else for(; y < ymax; y++) {
            int32_t x1 = (p[0].y - y) * dx1;
            int32_t fx1 = fpmax - (((x1 << FRACTIONAL_BITS) / dy1) & fpmask);
            x1 = p[0].x + x1 / dy1;

            int32_t x2 = (p[3].y - y) * dx1 + dy1 - 1;
            int32_t fx2 = ((x2 << FRACTIONAL_BITS) / dy1) & fpmask;
            x2 = p[3].x + x2 / dy1;

            int16_t a = (dx1 < dy1) ? (fx1 * alpha / fpmax) : (alpha - astep1 + fx1 * dy1 / dx1 * alpha / fpmax);
            for(uint32_t i = x1++; a > 0 && i >= p[0].x; i--) {
                if(IN_X_CLIP(g, i)) BLEND_PIXEL(PIXEL(g, i, y), a, fg);
                a -= astep1;
            }

            a = (dx1 < dy1) ? (fx2 * alpha / fpmax) : (alpha - astep1 + fx2 * dy1 / dx1 * alpha / fpmax);
            for(uint32_t i = x2--; a > 0 && i <= p[2].x; i++) {
                if(IN_X_CLIP(g, i)) BLEND_PIXEL(PIXEL(g, i, y), a, fg);
                a -= astep1;
            }

            if(alpha == 0x1F) SET_H_LINE(g, color, F_X1(g, x1), F_X2(g, x2), y);
            else BLEND_H_LINE(g, alpha, fg, F_X1(g, x1), F_X2(g, x2), y);
        }
        ymax = F_Y2(g, p[3].y);
        for(; y <= ymax; y++) {
            int32_t x1 = (y - p[0].y) * dx2;
            int32_t fx1 = fpmax - (((x1 << FRACTIONAL_BITS) / dy2) & fpmask);
            x1 = p[0].x + x1 / dy2;

            int32_t x2 = (p[3].y - y) * dx1 + dy1 - 1;
            int32_t fx2 = ((x2 << FRACTIONAL_BITS) / dy1) & fpmask;
            x2 = p[3].x + x2 / dy1;

            int16_t a = (dx2 < dy2) ? (fx1 * alpha / fpmax) : (alpha - astep2 + fx1 * dy2 / dx2 * alpha / fpmax);
            for(uint32_t i = x1++; a > 0 && i >= p[0].x; i--) {
                if(IN_X_CLIP(g, i)) BLEND_PIXEL(PIXEL(g, i, y), a, fg);
                a -= astep2;
            }

            a = (dx1 < dy1) ? (fx2 * alpha / fpmax) : (alpha - astep1 + fx2 * dy1 / dx1 * alpha / fpmax);
            for(uint32_t i = x2--; a > 0 && i <= p[2].x; i++) {
                if(IN_X_CLIP(g, i)) BLEND_PIXEL(PIXEL(g, i, y), a, fg);
                a -= astep1;
            }
            if(alpha == 0x1F) SET_H_LINE(g, color, F_X1(g, x1), F_X2(g, x2), y);
            else BLEND_H_LINE(g, alpha, fg, F_X1(g, x1), F_X2(g, x2), y);
        }
    }
}

void Rgb565_DrawLine(Gfx *g, uint32_t color, uint32_t thickness, int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
    if(x1 == x2 && y1 == y2)
        return;
    else if(y1 == y2)
        Rgb565_DrawHLine(g, color, thickness, x1, x2, y1);
    else if(x1 == x2)
        Rgb565_DrawVLine(g, color, thickness, y1, y2, x1);
    else
        Rgb565_DrawSkewLine(g, color, thickness, x1, y1, x2, y2);
}
