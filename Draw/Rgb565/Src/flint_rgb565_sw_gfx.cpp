
#include <string.h>
#include "flint_rgb565_gfx.h"
#include "flint_rgb565_gfx_helper.h"

static constexpr uint16_t fpmax = (1 << GFX_F_BITS);
static constexpr uint16_t fphalf = (1 << (GFX_F_BITS - 1));
static constexpr uint16_t fpmask = fpmax - 1;

typedef struct {
    int32_t x;
    int32_t y;
} Point;

static uint64_t ISqrt(uint64_t x) {
    uint64_t g, ret = 1ull << ((63 - __builtin_clzll(x)) >> 1);
    do {
        g = ret;
        ret = (g + x / g) >> 1;
    } while(GFX_ABS((int64_t)(ret - g)) > 1);
    return ret;
}

static inline __attribute__((always_inline)) bool IsVisible(Rgb565GfxHelper *g, int32_t x, int32_t y, int32_t w, int32_t h) {
    if(x > g->clipX2 || (x + w) < g->clipX1) return false;
    if(y > g->clipY2 || (y + h) < g->clipY1) return false;
    return true;
}

static void FillCornerArc(Rgb565GfxHelper *g, uint32_t color, int32_t cx, int32_t cy, uint32_t r, uint8_t arc) {
    if(arc == 0) { if(!IsVisible(g, cx - r, cy - r, r, r)) return; }
    else if(arc == 1) { if(!IsVisible(g, cx, cy - r, r, r)) return; }
    else if(arc == 2) { if(!IsVisible(g, cx, cy, r, r)) return; }
    else if(!IsVisible(g, cx - r, cy, r, r)) return;
    int32_t y = (arc < 2) ? GFX_MAX(0, cy - g->clipY2) : GFX_MAX(1, g->clipY1 - cy);
    int32_t ymax = (arc < 2) ? (cy - g->clipY1) : (g->clipY2 - cy);
    int64_t rr = (int64_t)r * r;
    int32_t x = r;
    uint8_t x0 = (arc == 0 || arc == 3) ? 0 : 1;
    uint8_t alpha = color >> 27;
    for(; y < x; y++) {
        if(y > ymax) return;
        int64_t fx = ISqrt((rr - (int64_t)y * y) << (GFX_F_BITS << 1));
        uint8_t a = ((uint32_t)fx & fpmask) * alpha / fpmax;
        x = (int32_t)(fx >> GFX_F_BITS);
        int32_t py = (arc < 2) ? (cy - y) : (cy + y);
        if(arc == 0 || arc == 3) {
            g->blendHLine(alpha, color, cx - x, cx, py);
            if(a > 0) g->blendPixel(a, color, cx - x - 1, py);
        }
        else {
            g->blendHLine(alpha, color, cx + x0, cx + x, py);
            if(a > 0) g->blendPixel(a, color, cx + x + 1, py);
        }
    }
    x = (int32_t)(ISqrt((rr - (int64_t)y * y) << (GFX_F_BITS << 1)) >> GFX_F_BITS);
    x = (arc == 0 || arc == 3) ? GFX_MIN(x, cx - g->clipX1) : GFX_MIN(x, g->clipX2 - cx);
    x0 = (arc == 0 || arc == 3) ? GFX_MAX(x0, cx - g->clipX2) : GFX_MAX(x0, g->clipX1 - cx);
    int32_t y0 = y;
    for(; x >= x0; x--) {
        int64_t fy = ISqrt((rr - (int64_t)x * x) << (GFX_F_BITS << 1));
        uint8_t a = ((uint32_t)fy & fpmask) * alpha / fpmax;
        y = (int32_t)(fy >> GFX_F_BITS);
        int32_t px = (arc == 0 || arc == 3) ? (cx - x) : (cx + x);
        if(arc < 2) {
            g->blendVLine(alpha, color, cy - y, cy - y0, px);
            if(a > 0) g->blendPixel(a, color, px, cy - y - 1);
        }
        else {
            g->blendVLine(alpha, color, cy + y0, cy + y, px);
            if(a > 0) g->blendPixel(a, color, px, cy + y + 1);
        }
    }
}

static void CalcRectPoints(uint32_t width, int32_t x1, int32_t y1, int32_t x2, int32_t y2, Point *points) {
    static const uint8_t wratio[] = {
        255, 251, 247, 244, 240, 237, 234, 231, 228, 225, 222, 220, 217, 215, 212, 210,
        208, 206, 203, 201, 199, 198, 196, 194, 192, 190, 189, 187, 186, 184, 182, 181,
        180, 178, 177, 176, 174, 173, 172, 170, 169, 168, 167, 166, 165, 164, 163, 162,
        161, 160, 159, 158, 157, 156, 155, 154, 153, 152, 151, 150, 150, 149, 148, 147,
        255
    };
    int32_t dx = GFX_ABS(x2 - x1);
    int32_t dy = GFX_ABS(y2 - y1);
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
        GFX_SWAP(x1, x2);
        GFX_SWAP(y1, y2);
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

static void DrawChar(Rgb565GfxHelper *g, const CharInfo *c, uint32_t color, int32_t x, int32_t y) {
    uint8_t width = c->getWidth();

    int32_t cx0 = GFX_MAX(g->clipX1 - x, 0);
    int32_t cxw = GFX_MIN(g->clipX2 - x + 1, width);
    if(cx0 >= cxw) return;

    uint8_t height = c->getHeight();
    int8_t yOff = c->getYOffset();
    uint8_t alpha = color >> 27;

    if(alpha == 0x1F) {
        for(int32_t cy = 0; cy < height; cy++) {
            int32_t py = y + cy + yOff;
            if(!(g->clipY1 <= y && y <= g->clipY2)) continue;
            uint16_t *p = &((uint16_t *)g->data)[py * g->w + x];
            for(int32_t cx = cx0; cx < cxw; cx++)
                if(c->getPixel(cx, cy)) p[cx] = color;
        }
    }
    else {
        for(int32_t cy = 0; cy < height; cy++) {
            int32_t py = y + cy + yOff;
            if(!(g->clipY1 <= y && y <= g->clipY2)) continue;
            for(int32_t cx = cx0; cx < cxw; cx++)
                if(c->getPixel(cx, cy)) g->blendPixel(alpha, color, x + cx, py);
        }
    }
}

Rgb565Gfx::Rgb565Gfx(int32_t w, int32_t h, int32_t clipX1, int32_t clipY1, int32_t clipX2, int32_t clipY2, uint8_t *data) :
w(w), h(h), clipX1(clipX1), clipY1(clipY1), clipX2(clipX2), clipY2(clipY2), data(data) {

}

void Rgb565Gfx::clear(uint32_t color) {
    ((Rgb565GfxHelper *)this)->clear(color);
}

void Rgb565Gfx::drawLine(uint32_t color, uint32_t thickness, int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
    if(x1 == x2 && y1 == y2)
        return;
    uint8_t alpha = color >> 27;
    if(y1 == y2) {
        if(x1 > x2) GFX_SWAP(x1, x2);
        int32_t half = (thickness - 1) >> 1;
        y1 -= half;
        y2 += half;

        if((thickness & 0x01) == 0) {
            uint32_t a1 = color >> 28;
            uint32_t a2 = color >> 29;

            ((Rgb565GfxHelper *)this)->blendVLine(a1, color, y1, y2, x1);
            ((Rgb565GfxHelper *)this)->blendVLine(a1, color, y1, y2, x2);

            ((Rgb565GfxHelper *)this)->blendHLine(a1, color, x1 + 1, x2 - 1, y1 - 1);
            ((Rgb565GfxHelper *)this)->blendHLine(a1, color, x1 + 1, x2 - 1, y2 + 1);

            ((Rgb565GfxHelper *)this)->blendPixel(a2, color, x1, y1 - 1);
            ((Rgb565GfxHelper *)this)->blendPixel(a2, color, x2, y1 - 1);
            ((Rgb565GfxHelper *)this)->blendPixel(a2, color, x1, y2 + 1);
            ((Rgb565GfxHelper *)this)->blendPixel(a2, color, x2, y2 + 1);

            x1++;
            x2--;
        }
        ((Rgb565GfxHelper *)this)->blendRect(alpha, color, x1, y1, x2, y2);
    }
    else if(x1 == x2) {
        if(y1 > y2) GFX_SWAP(y1, y2);
        int32_t half = (thickness - 1) >> 1;
        x1 -= half;
        x2 += half;

        if((thickness & 0x01) == 0) {
            uint32_t a1 = color >> 28;
            uint32_t a2 = color >> 29;

            ((Rgb565GfxHelper *)this)->blendHLine(a1, color, x1, x2, y1);
            ((Rgb565GfxHelper *)this)->blendHLine(a1, color, x1, x2, y2);

            ((Rgb565GfxHelper *)this)->blendVLine(a1, color, y1 + 1, y2 - 1, x1 - 1);
            ((Rgb565GfxHelper *)this)->blendVLine(a1, color, y1 + 1, y2 - 1, x2 + 1);

            ((Rgb565GfxHelper *)this)->blendPixel(a2, color, x1 - 1, y1);
            ((Rgb565GfxHelper *)this)->blendPixel(a2, color, x2 + 1, y1);
            ((Rgb565GfxHelper *)this)->blendPixel(a2, color, x1 - 1, y2);
            ((Rgb565GfxHelper *)this)->blendPixel(a2, color, x2 + 1, y2);

            y1++;
            y2--;
        }
        ((Rgb565GfxHelper *)this)->blendRect(alpha, color, x1, y1, x2, y2);
    }
    else {
        if(thickness == 1) {
            int32_t err = 0;
            int32_t dx = GFX_ABS(x2 - x1);
            int32_t dy = GFX_ABS(y2 - y1);
            if(dx > dy) {
                int8_t ystep = (y1 < y2) ? 1 : -1;
                int32_t y = y1;
                if(x1 > x2) {
                    GFX_SWAP(x1, x2);
                    GFX_SWAP(y1, y2);
                }
                for(int32_t x = x1; x <= x2; x++) {
                    uint8_t a = err * alpha / dx;
                    ((Rgb565GfxHelper *)this)->blendPixel(alpha - a, color, x, y);
                    ((Rgb565GfxHelper *)this)->blendPixel(a, color, x, y - ystep);
                    err -= dy;
                    if(err < 0) {
                        y += ystep;
                        err += dx;
                    }
                }
            }
            else {
                int8_t xstep = (x1 < x2) ? 1 : -1;
                int32_t x = x1;
                if(y1 > y2) {
                    GFX_SWAP(x1, x2);
                    GFX_SWAP(y1, y2);
                }
                for(int32_t y = y1; y <= y2; y++) {
                    uint8_t a = err * alpha / dy;
                    ((Rgb565GfxHelper *)this)->blendPixel(alpha - a, color, x, y);
                    ((Rgb565GfxHelper *)this)->blendPixel(a, color, x - xstep, y);
                    err -= dx;
                    if(err < 0) {
                        x += xstep;
                        err += dy;
                    }
                }
            }
        }
        else {
            Point p[4];
            CalcRectPoints(thickness, x1, y1, x2, y2, p);

            int32_t ymax = GFX_MIN(this->clipY2, GFX_MIN(p[0].y, p[2].y));
            int32_t y = GFX_MAX(this->clipY1, p[1].y);

            int32_t dx1 = p[1].x - p[0].x;
            int32_t dy1 = p[0].y - p[1].y;
            int32_t dx2 = p[2].x - p[1].x;
            int32_t dy2 = p[2].y - p[1].y;
            uint8_t astep1 = (alpha * dy1 + (dx1 >> 1)) / dx1;
            uint8_t astep2 = (alpha * dy2 + (dx2 >> 1)) / dx2;

            for(; y <= ymax; y++) {
                int32_t x1 = (p[0].y - y) * dx1;
                int32_t fx1 = fpmax - (((x1 << GFX_F_BITS) / dy1) & fpmask);
                x1 = p[0].x + x1 / dy1;

                int32_t x2 = (y - p[1].y) * dx2 + dy2 - 1;
                int32_t fx2 = ((x2 << GFX_F_BITS) / dy2) & fpmask;
                x2 = p[1].x + x2 / dy2;

                int16_t a = (dx1 < dy1) ? (fx1 * alpha / fpmax) : (alpha - astep1 + fx1 * dy1 / dx1 * alpha / fpmax);
                for(uint32_t i = x1++; a > 0 && i >= p[0].x; i--) {
                    ((Rgb565GfxHelper *)this)->blendPixel(a, color, i, y);
                    a -= astep1;
                }

                a = (dx2 < dy2) ? (fx2 * alpha / fpmax) : (alpha - astep2 + fx2 * dy2 / dx2 * alpha / fpmax);
                for(uint32_t i = x2--; a > 0 && i <= p[2].x; i++) {
                    ((Rgb565GfxHelper *)this)->blendPixel(a, color, i, y);
                    a -= astep2;
                }

                ((Rgb565GfxHelper *)this)->blendHLine(alpha, color, x1, x2, y);
            }
            ymax = GFX_MIN(this->clipY2, GFX_MAX(p[0].y, p[2].y));
            if(p[0].y < p[2].y) for(; y < ymax; y++) {
                int32_t x1 = (y - p[0].y) * dx2;
                int32_t fx1 = fpmax - (((x1 << GFX_F_BITS) / dy2) & fpmask);
                x1 = p[0].x + x1 / dy2;

                int32_t x2 = (y - p[1].y) * dx2 + dy2 - 1;
                int32_t fx2 = ((x2 << GFX_F_BITS) / dy2) & fpmask;
                x2 = p[1].x + x2 / dy2;

                int16_t a = (dx2 < dy2) ? (fx1 * alpha / fpmax) : (alpha - astep2 + fx1 * dy2 / dx2 * alpha / fpmax);
                for(uint32_t i = x1++; a > 0 && i >= p[0].x; i--) {
                    ((Rgb565GfxHelper *)this)->blendPixel(a, color, i, y);
                    a -= astep2;
                }

                a = (dx2 < dy2) ? (fx2 * alpha / fpmax) : (alpha - astep2 + fx2 * dy2 / dx2 * alpha / fpmax);
                for(uint32_t i = x2--; a > 0 && i <= p[2].x; i++) {
                    ((Rgb565GfxHelper *)this)->blendPixel(a, color, i, y);
                    a -= astep2;
                }

                ((Rgb565GfxHelper *)this)->blendHLine(alpha, color, x1, x2, y);
            }
            else for(; y < ymax; y++) {
                int32_t x1 = (p[0].y - y) * dx1;
                int32_t fx1 = fpmax - (((x1 << GFX_F_BITS) / dy1) & fpmask);
                x1 = p[0].x + x1 / dy1;

                int32_t x2 = (p[3].y - y) * dx1 + dy1 - 1;
                int32_t fx2 = ((x2 << GFX_F_BITS) / dy1) & fpmask;
                x2 = p[3].x + x2 / dy1;

                int16_t a = (dx1 < dy1) ? (fx1 * alpha / fpmax) : (alpha - astep1 + fx1 * dy1 / dx1 * alpha / fpmax);
                for(uint32_t i = x1++; a > 0 && i >= p[0].x; i--) {
                    ((Rgb565GfxHelper *)this)->blendPixel(a, color, i, y);
                    a -= astep1;
                }

                a = (dx1 < dy1) ? (fx2 * alpha / fpmax) : (alpha - astep1 + fx2 * dy1 / dx1 * alpha / fpmax);
                for(uint32_t i = x2--; a > 0 && i <= p[2].x; i++) {
                    ((Rgb565GfxHelper *)this)->blendPixel(a, color, i, y);
                    a -= astep1;
                }

                ((Rgb565GfxHelper *)this)->blendHLine(alpha, color, x1, x2, y);
            }
            ymax = GFX_MIN(this->clipY2, p[3].y);
            for(; y <= ymax; y++) {
                int32_t x1 = (y - p[0].y) * dx2;
                int32_t fx1 = fpmax - (((x1 << GFX_F_BITS) / dy2) & fpmask);
                x1 = p[0].x + x1 / dy2;

                int32_t x2 = (p[3].y - y) * dx1 + dy1 - 1;
                int32_t fx2 = ((x2 << GFX_F_BITS) / dy1) & fpmask;
                x2 = p[3].x + x2 / dy1;

                int16_t a = (dx2 < dy2) ? (fx1 * alpha / fpmax) : (alpha - astep2 + fx1 * dy2 / dx2 * alpha / fpmax);
                for(uint32_t i = x1++; a > 0 && i >= p[0].x; i--) {
                    ((Rgb565GfxHelper *)this)->blendPixel(a, color, i, y);
                    a -= astep2;
                }

                a = (dx1 < dy1) ? (fx2 * alpha / fpmax) : (alpha - astep1 + fx2 * dy1 / dx1 * alpha / fpmax);
                for(uint32_t i = x2--; a > 0 && i <= p[2].x; i++) {
                    ((Rgb565GfxHelper *)this)->blendPixel(a, color, i, y);
                    a -= astep1;
                }

                ((Rgb565GfxHelper *)this)->blendHLine(alpha, color, x1, x2, y);
            }
        }
    }
}

void Rgb565Gfx::drawRect(uint32_t color, uint32_t thickness, int32_t x, int32_t y, int32_t w, int32_t h) {
    uint8_t alpha = color >> 27;
    int32_t half = (thickness - 1) >> 1;

    int32_t x1 = x;
    int32_t x2 = x + w;
    int32_t y1 = y;
    int32_t y2 = y + h;

    if((thickness & 0x01) == 0) {
        uint8_t a = color >> 28;

        ((Rgb565GfxHelper *)this)->blendHLine(a, color, x1 - half - 1, x2 + half + 1, y1 - half - 1);
        ((Rgb565GfxHelper *)this)->blendHLine(a, color, x1 - half - 1, x2 + half + 1, y2 + half + 1);

        ((Rgb565GfxHelper *)this)->blendHLine(a, color, x1 + half + 1, x2 - half - 1, y1 + half + 1);
        ((Rgb565GfxHelper *)this)->blendHLine(a, color, x1 + half + 1, x2 - half - 1, y2 - half - 1);

        ((Rgb565GfxHelper *)this)->blendVLine(a, color, y1 - half, y2 + half, x1 - half - 1);
        ((Rgb565GfxHelper *)this)->blendVLine(a, color, y1 - half, y2 + half, x2 + half + 1);

        ((Rgb565GfxHelper *)this)->blendVLine(a, color, y1 + half + 1, y2 - half - 1, x1 + half + 1);
        ((Rgb565GfxHelper *)this)->blendVLine(a, color, y1 + half + 1, y2 - half - 1, x2 - half - 1);

        thickness--;
    }

    ((Rgb565GfxHelper *)this)->blendRect(alpha, color, x1 - half, y1 - half, x2 + half, y1 + half);
    ((Rgb565GfxHelper *)this)->blendRect(alpha, color, x1 - half, y2 - half, x2 + half, y2 + half);

    ((Rgb565GfxHelper *)this)->blendRect(alpha, color, x1 - half, y1 + half + 1, x1 - half, y2 - half - 1);
    ((Rgb565GfxHelper *)this)->blendRect(alpha, color, x2 + half, y1 + half + 1, x2 + half, y2 - half - 1);
}

void Rgb565Gfx::fillRect(uint32_t color, int32_t x, int32_t y, int32_t w, int32_t h) {
    ((Rgb565GfxHelper *)this)->blendRect(color >> 27, color, x, y, x + w, y + h);
}

void Rgb565Gfx::fillRoundRect(uint32_t color, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t r1, uint32_t r2, uint32_t r3, uint32_t r4) {
    if(!IsVisible((Rgb565GfxHelper *)this, x, y, w, h)) return;

    uint8_t alpha = color >> 27;

    uint8_t scale = 128;
    if(r1 + r2 > w) scale = (w << 7) / (r1 + r2);
    if(r3 + r4 > w) scale = GFX_MIN(scale, (w << 7) / (r3 + r4));
    if(r1 + r4 > h) scale = GFX_MIN(scale, (h << 7) / (r1 + r4));
    if(r2 + r3 > h) scale = GFX_MIN(scale, (h << 7) / (r2 + r3));
    if(scale < 128) {
        r1 = r1 * scale >> 7;
        r2 = r2 * scale >> 7;
        r3 = r3 * scale >> 7;
        r4 = r4 * scale >> 7;
    }

    int32_t x1, x2;
    int32_t y1 = GFX_MAX(clipY1 - y, 0);
    int32_t y2 = GFX_MIN(clipY2 - y, h - 1);

    for(int32_t i = y1; i <= y2; i++) {
        x1 = x + (i <= r1 ? (r1 + 1) : (i > (h - r4 - 1) ? (r4 + 1) : 0));
        x2 = x + w - 1 - (i <= r2 ? r2 : (i > (h - r3 - 1) ? r3 : 0));
        ((Rgb565GfxHelper *)this)->blendHLine(alpha, color, x1, x2, i + y);
    }

    x2 = x + w - 1;
    y2 = y + h - 1;
    if(r1 > 0) FillCornerArc((Rgb565GfxHelper *)this, color, x + r1, y + r1, r1, 0);
    if(r2 > 0) FillCornerArc((Rgb565GfxHelper *)this, color, x2 - r2, y + r2, r2, 1);
    if(r3 > 0) FillCornerArc((Rgb565GfxHelper *)this, color, x2 - r3, y2 - r3, r3, 2);
    if(r4 > 0) FillCornerArc((Rgb565GfxHelper *)this, color, x + r4, y2 - r4, r4, 3);
}

void Rgb565Gfx::drawEllipse(uint32_t color, uint32_t thickness, int32_t x, int32_t y, uint32_t w, uint32_t h) {
    if(w <= (thickness + 2) || h <= (thickness + 2))
        return fillEllipse(color, x - thickness / 2, y - thickness / 2, w + thickness, h + thickness);

    if(thickness == 1) {
        if(!IsVisible((Rgb565GfxHelper *)this, x, y, w, h)) return;

        uint8_t alpha = color >> 27;

        int32_t cx = x + w / 2;
        int32_t cy = y + h / 2;

        uint64_t aa = ((uint64_t)w * w) << 2;
        uint64_t bb = ((uint64_t)h * h) << 2;
        uint64_t ab = (uint64_t)w * w * h * h;

        int32_t xo = w / 2;
        int32_t yo = (cy > clipY2) ? (cy - clipY2) : ((clipY1 > cy) ? (clipY1 - cy) : 1);
        int32_t ymax = GFX_MAX(GFX_ABS(cy - clipY1), GFX_ABS(clipY2 - cy));

        if(w & 1) {
            uint8_t a = (alpha + 1) / 2;
            ((Rgb565GfxHelper *)this)->blendPixel(a, color, x, cy);
            ((Rgb565GfxHelper *)this)->blendPixel(alpha - a, color, x + 1, cy);
            ((Rgb565GfxHelper *)this)->blendPixel(a, color, x + w - 1, cy);
            ((Rgb565GfxHelper *)this)->blendPixel(a, color, x + w - 2, cy);
        }
        else {
            ((Rgb565GfxHelper *)this)->blendPixel(alpha, color, x + 1, cy);
            ((Rgb565GfxHelper *)this)->blendPixel(alpha, color, x + w - 1, cy);
        }
        for(; bb * xo >= aa * yo; yo++) {
            if(yo > ymax) return;
            int64_t fxo = ISqrt(((ab - aa * yo * yo) / bb) << (GFX_F_BITS << 1));
            xo = (int32_t)(fxo >> GFX_F_BITS);
            uint8_t ao = ((uint32_t)fxo & fpmask) * alpha / fpmax;
            uint8_t ai = alpha - ao;
            if(ao > 0) {
                ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx - xo, cy - yo);
                ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx - xo, cy + yo);
                ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx + xo, cy - yo);
                ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx + xo, cy + yo);
            }
            if(ai > 0) {
                ((Rgb565GfxHelper *)this)->blendPixel(ai, color, cx - xo + 1, cy - yo);
                ((Rgb565GfxHelper *)this)->blendPixel(ai, color, cx - xo + 1, cy + yo);
                ((Rgb565GfxHelper *)this)->blendPixel(ai, color, cx + xo - 1, cy - yo);
                ((Rgb565GfxHelper *)this)->blendPixel(ai, color, cx + xo - 1, cy + yo);
            }
        }
        xo = (int32_t)(ISqrt(((ab - aa * yo * yo) / bb) << (GFX_F_BITS << 1)) >> GFX_F_BITS);
        int32_t xmin = (cx > clipX2) ? (cx - clipX2) : ((clipX1 > cx) ? (clipX1 - cx) : 1);
        int32_t xmax = GFX_MAX(GFX_ABS(cx - clipX1), GFX_ABS(clipX2 - cx));
        xo = GFX_MIN(xo, xmax);
        for(; xo >= xmin; xo--) {
            int64_t fyo = ISqrt(((ab - bb * xo * xo) / aa) << (GFX_F_BITS << 1));
            yo = (int32_t)(fyo >> GFX_F_BITS);
            uint8_t ao = ((uint32_t)fyo & fpmask) * alpha / fpmax;
            uint8_t ai = alpha - ao;
            if(ao > 0) {
                ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx - xo, cy - yo);
                ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx - xo, cy + yo);
                ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx + xo, cy - yo);
                ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx + xo, cy + yo);
            }
            if(ai > 0) {
                ((Rgb565GfxHelper *)this)->blendPixel(ai, color, cx - xo, cy - yo + 1);
                ((Rgb565GfxHelper *)this)->blendPixel(ai, color, cx - xo, cy + yo - 1);
                ((Rgb565GfxHelper *)this)->blendPixel(ai, color, cx + xo, cy - yo + 1);
                ((Rgb565GfxHelper *)this)->blendPixel(ai, color, cx + xo, cy + yo - 1);
            }
        }
        if(h & 1) {
            uint8_t a = (alpha + 1) / 2;
            ((Rgb565GfxHelper *)this)->blendPixel(a, color, cx, y);
            ((Rgb565GfxHelper *)this)->blendPixel(alpha - a, color, cx, y + 1);
            ((Rgb565GfxHelper *)this)->blendPixel(a, color, cx, y + h - 1);
            ((Rgb565GfxHelper *)this)->blendPixel(a, color, cx, y + h - 2);
        }
        else {
            ((Rgb565GfxHelper *)this)->blendPixel(alpha, color, cx, y + 1);
            ((Rgb565GfxHelper *)this)->blendPixel(alpha, color, cx, y + h - 1);
        }
    }
    else {
        int32_t cx = x + w / 2;
        int32_t cy = y + h / 2;

        int32_t wo = w + thickness;
        int32_t ho = h + thickness;

        if(!IsVisible((Rgb565GfxHelper *)this, cx - wo / 2, cy - ho / 2, wo, ho)) return;

        uint8_t alpha = color >> 27;

        int32_t wi = (w > thickness) ? (w - thickness) : 1;
        int32_t hi = (h > thickness) ? (h - thickness) : 1;

        uint64_t aa = ((uint64_t)wo * wo) << 2;
        uint64_t bb = ((uint64_t)ho * ho) << 2;
        uint64_t ab = (uint64_t)wo * wo * ho * ho;

        uint64_t cc = ((uint64_t)wi * wi) << 2;
        uint64_t dd = ((uint64_t)hi * hi) << 2;
        uint64_t cd = (uint64_t)wi * wi * hi * hi;

        int32_t xo = wo / 2, xi = wi / 2;
        int32_t yo = (cy > clipY2) ? (cy - clipY2) : ((clipY1 > cy) ? (clipY1 - cy) : 1);
        int32_t ymax = GFX_MAX(GFX_ABS(cy - clipY1), GFX_ABS(clipY2 - cy));

        ((Rgb565GfxHelper *)this)->blendHLine(alpha, color, cx - wo / 2 + 1, cx - wi / 2 - (wi & 1), cy);
        ((Rgb565GfxHelper *)this)->blendHLine(alpha, color, cx + wi / 2 + (wi & 1), cx + wo / 2 - 1, cy);
        if(wo & 1) {
            uint8_t ao = (alpha + 1) / 2;
            ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx - wo / 2, cy);
            ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx + wo / 2, cy);
        }
        if(wi & 1) {
            uint8_t ai = (alpha + 1) / 2;
            ((Rgb565GfxHelper *)this)->blendPixel(ai, color, cx - wi / 2, cy);
            ((Rgb565GfxHelper *)this)->blendPixel(ai, color, cx + wi / 2, cy);
        }
        for(; dd * xi >= cc * yo; yo++) {
            if(yo > ymax) return;
            int64_t fxo = ISqrt(((ab - aa * yo * yo) / bb) << (GFX_F_BITS << 1));
            int64_t fxi = ISqrt(((cd - cc * yo * yo) / dd) << (GFX_F_BITS << 1));
            xo = (int32_t)(fxo >> GFX_F_BITS);
            xi = (int32_t)(fxi >> GFX_F_BITS);
            uint8_t ao = ((uint32_t)fxo & fpmask) * alpha / fpmax;
            uint8_t ai = alpha - ((uint32_t)fxi & fpmask) * alpha / fpmax;
            ((Rgb565GfxHelper *)this)->blendHLine(alpha, color, cx - xo + 1, cx - xi - 1, cy - yo);
            ((Rgb565GfxHelper *)this)->blendHLine(alpha, color, cx - xo + 1, cx - xi - 1, cy + yo);
            ((Rgb565GfxHelper *)this)->blendHLine(alpha, color, cx + xi + 1, cx + xo - 1, cy - yo);
            ((Rgb565GfxHelper *)this)->blendHLine(alpha, color, cx + xi + 1, cx + xo - 1, cy + yo);
            if(ao > 0) {
                ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx - xo, cy - yo);
                ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx - xo, cy + yo);
                ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx + xo, cy - yo);
                ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx + xo, cy + yo);
            }
            if(ai > 0 && xi > 0) {
                ((Rgb565GfxHelper *)this)->blendPixel(ai, color, cx - xi, cy - yo);
                ((Rgb565GfxHelper *)this)->blendPixel(ai, color, cx - xi, cy + yo);
                ((Rgb565GfxHelper *)this)->blendPixel(ai, color, cx + xi, cy - yo);
                ((Rgb565GfxHelper *)this)->blendPixel(ai, color, cx + xi, cy + yo);
            }
        }

        int32_t xo0 = GFX_MAX(xi, 1);
        for(; bb * xo >= aa * yo; yo++) {
            if(yo > ymax) return;
            int64_t fxo = ISqrt(((ab - aa * yo * yo) / bb) << (GFX_F_BITS << 1));
            xo = (int32_t)(fxo >> GFX_F_BITS);
            uint8_t ao = ((uint32_t)fxo & fpmask) * alpha / fpmax;
            ((Rgb565GfxHelper *)this)->blendHLine(alpha, color, cx - xo + 1, cx - xo0, cy - yo);
            ((Rgb565GfxHelper *)this)->blendHLine(alpha, color, cx - xo + 1, cx - xo0, cy + yo);
            ((Rgb565GfxHelper *)this)->blendHLine(alpha, color, cx + xo0, cx + xo - 1, cy - yo);
            ((Rgb565GfxHelper *)this)->blendHLine(alpha, color, cx + xo0, cx + xo - 1, cy + yo);
            if(ao > 0) {
                ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx - xo, cy - yo);
                ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx - xo, cy + yo);
                ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx + xo, cy - yo);
                ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx + xo, cy + yo);
            }
        }

        int32_t yo0 = yo;
        xo = (int32_t)(ISqrt(((ab - aa * yo0 * yo0) / bb) << (GFX_F_BITS << 1)) >> GFX_F_BITS);
        int32_t xmin = (cx > clipX2) ? (cx - clipX2) : ((clipX1 > cx) ? (clipX1 - cx) : 1);
        int32_t xmax = GFX_MAX(GFX_ABS(cx - clipX1), GFX_ABS(clipX2 - cx));
        xo = GFX_MIN(xo, xmax);
        xo0 = GFX_MAX(xo0, xmin);
        for(; xo >= xo0; xo--) {
            int64_t fyo = ISqrt(((ab - bb * xo * xo) / aa) << (GFX_F_BITS << 1));
            yo = (int32_t)(fyo >> GFX_F_BITS);
            uint8_t ao = ((uint32_t)fyo & fpmask) * alpha / fpmax;
            ((Rgb565GfxHelper *)this)->blendVLine(alpha, color, cy - yo + 1, cy - yo0, cx - xo);
            ((Rgb565GfxHelper *)this)->blendVLine(alpha, color, cy - yo + 1, cy - yo0, cx + xo);
            ((Rgb565GfxHelper *)this)->blendVLine(alpha, color, cy + yo0, cy + yo - 1, cx - xo);
            ((Rgb565GfxHelper *)this)->blendVLine(alpha, color, cy + yo0, cy + yo - 1, cx + xo);
            if(ao > 0) {
                ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx - xo, cy - yo);
                ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx + xo, cy - yo);
                ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx - xo, cy + yo);
                ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx + xo, cy + yo);
            }
        }

        for(; xo >= xmin; xo--) {
            int64_t fyo = ISqrt(((ab - bb * xo * xo) / aa) << (GFX_F_BITS << 1));
            int64_t fyi = ISqrt(((cd - dd * xo * xo) / cc) << (GFX_F_BITS << 1));
            yo = (int32_t)(fyo >> GFX_F_BITS);
            int32_t yi = (int32_t)(fyi >> GFX_F_BITS);
            uint8_t ao = ((uint32_t)fyo & fpmask) * alpha / fpmax;
            uint8_t ai = alpha - ((uint32_t)fyi & fpmask) * alpha / fpmax;
            ((Rgb565GfxHelper *)this)->blendVLine(alpha, color, cy - yo + 1, cy - yi - 1, cx - xo);
            ((Rgb565GfxHelper *)this)->blendVLine(alpha, color, cy - yo + 1, cy - yi - 1, cx + xo);
            ((Rgb565GfxHelper *)this)->blendVLine(alpha, color, cy + yi + 1, cy + yo - 1, cx - xo);
            ((Rgb565GfxHelper *)this)->blendVLine(alpha, color, cy + yi + 1, cy + yo - 1, cx + xo);
            if(ao > 0) {
                ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx - xo, cy - yo);
                ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx + xo, cy - yo);
                ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx - xo, cy + yo);
                ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx + xo, cy + yo);
            }
            if(ai > 0 && yi > 0) {
                ((Rgb565GfxHelper *)this)->blendPixel(ai, color, cx - xo, cy - yi);
                ((Rgb565GfxHelper *)this)->blendPixel(ai, color, cx + xo, cy - yi);
                ((Rgb565GfxHelper *)this)->blendPixel(ai, color, cx - xo, cy + yi);
                ((Rgb565GfxHelper *)this)->blendPixel(ai, color, cx + xo, cy + yi);
            }
        }
        ((Rgb565GfxHelper *)this)->blendVLine(alpha, color, cy - ho / 2 + 1, cy - hi / 2 - (hi & 1), cx);
        ((Rgb565GfxHelper *)this)->blendVLine(alpha, color, cy + hi / 2 + (hi & 1), cy + ho / 2 - 1, cx);
        if(ho & 1) {
            uint8_t ao = (alpha + 1) / 2;
            ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx, cy - ho / 2);
            ((Rgb565GfxHelper *)this)->blendPixel(ao, color, cx, cy + ho / 2);
        }
        if(hi & 1) {
            uint8_t ai = (alpha + 1) / 2;
            ((Rgb565GfxHelper *)this)->blendPixel(ai, color, cx, cy - hi / 2);
            ((Rgb565GfxHelper *)this)->blendPixel(ai, color, cx, cy + hi / 2);
        }
    }
}

void Rgb565Gfx::fillEllipse(uint32_t color, int32_t x, int32_t y, uint32_t w, uint32_t h) {
    if(!IsVisible((Rgb565GfxHelper *)this, x, y, w, h)) return;

    uint8_t alpha = color >> 27;

    int32_t cx = x + w / 2;
    int32_t cy = y + h / 2;

    uint64_t ww = ((uint64_t)w * w) << 2;
    uint64_t hh = ((uint64_t)h * h) << 2;
    uint64_t wh = (uint64_t)w * w * h * h;

    int32_t ix = w;
    int32_t iy = (cy > clipY2) ? (cy - clipY2) : ((clipY1 > cy) ? (clipY1 - cy) : 1);
    int32_t ymax = GFX_MAX(GFX_ABS(cy - clipY1), GFX_ABS(clipY2 - cy));

    ((Rgb565GfxHelper *)this)->blendHLine(alpha, color, x + 1, x + w - 1 - (w & 1), cy);
    if(w & 1) {
        uint8_t a = (alpha + 1) / 2;
        ((Rgb565GfxHelper *)this)->blendPixel(a, color, x, cy);
        ((Rgb565GfxHelper *)this)->blendPixel(a, color, x + w - 1, cy);
    }
    for(; hh * ix >= ww * iy; iy++) {
        if(iy > ymax) return;
        int64_t fx = ISqrt(((wh - ww * iy * iy) / hh) << (GFX_F_BITS << 1));
        ix = (int32_t)(fx >> GFX_F_BITS);
        uint8_t a = ((uint32_t)fx & fpmask) * alpha / fpmax;
        ((Rgb565GfxHelper *)this)->blendHLine(alpha, color, cx - ix + 1, cx + ix - 1, cy - iy);
        ((Rgb565GfxHelper *)this)->blendHLine(alpha, color, cx - ix + 1, cx + ix - 1, cy + iy);
        if(a > 0) {
            ((Rgb565GfxHelper *)this)->blendPixel(a, color, cx - ix, cy - iy);
            ((Rgb565GfxHelper *)this)->blendPixel(a, color, cx + ix, cy - iy);
            ((Rgb565GfxHelper *)this)->blendPixel(a, color, cx - ix, cy + iy);
            ((Rgb565GfxHelper *)this)->blendPixel(a, color, cx + ix, cy + iy);
        }
    }
    ix = (int32_t)(ISqrt(((wh - ww * iy * iy) / hh) << (GFX_F_BITS << 1)) >> GFX_F_BITS);
    int32_t xmin = (cx > clipX2) ? (cx - clipX2) : ((clipX1 > cx) ? (clipX1 - cx) : 1);
    int32_t xmax = GFX_MAX(GFX_ABS(cx - clipX1), GFX_ABS(clipX2 - cx));
    ix = GFX_MIN(ix, xmax);
    for(; ix > 0; ix--) {
        int64_t fy = ISqrt(((wh - hh * ix * ix) / ww) << (GFX_F_BITS << 1));
        int32_t y2 = (int32_t)(fy >> GFX_F_BITS);
        uint8_t a = ((uint32_t)fy & fpmask) * alpha / fpmax;
        ((Rgb565GfxHelper *)this)->blendVLine(alpha, color, cy - y2 + 1, cy - iy, cx - ix);
        ((Rgb565GfxHelper *)this)->blendVLine(alpha, color, cy + iy, cy + y2 - 1, cx - ix);
        ((Rgb565GfxHelper *)this)->blendVLine(alpha, color, cy - y2 + 1, cy - iy, cx + ix);
        ((Rgb565GfxHelper *)this)->blendVLine(alpha, color, cy + iy, cy + y2 - 1, cx + ix);
        if(a > 0) {
            ((Rgb565GfxHelper *)this)->blendPixel(a, color, cx - ix, cy - y2);
            ((Rgb565GfxHelper *)this)->blendPixel(a, color, cx + ix, cy - y2);
            ((Rgb565GfxHelper *)this)->blendPixel(a, color, cx - ix, cy + y2);
            ((Rgb565GfxHelper *)this)->blendPixel(a, color, cx + ix, cy + y2);
        }
    }
    ((Rgb565GfxHelper *)this)->blendVLine(alpha, color, y + 1, cy - iy, cx);
    ((Rgb565GfxHelper *)this)->blendVLine(alpha, color, cy + iy, y + h - 1 - (h & 1), cx);
    if(h & 1) {
        uint8_t a = (alpha + 1) / 2;
        ((Rgb565GfxHelper *)this)->blendPixel(a, color, cx, y);
        ((Rgb565GfxHelper *)this)->blendPixel(a, color, cx, y + h - 1);
    }
}

void Rgb565Gfx::drawLatin1(uint8_t *str, uint32_t len, Font *font, uint32_t color, int32_t x, int32_t y) {
    uint8_t stdWidth = font->getStdWidth();
    uint8_t stdHeight = font->getStdHeight();
    uint8_t space = font->getSpaceWidth();
    space = GFX_MAX(space / 4, 1);

    while(len--) {
        if(x > clipX2) return;
        const CharInfo *c = font->getChar(*str++);
        if(c != NULL) {
            DrawChar((Rgb565GfxHelper *)this, c, color, x, y);
            x += c->getWidth() + space;
        }
        else {
            fillRect(color, x, y, stdWidth - 1, stdHeight - 1);
            x += stdWidth + space;
        }
    }
}

void Rgb565Gfx::drawUTF16(uint8_t *str, uint32_t len, Font *font, uint32_t color, int32_t x, int32_t y) {
    uint8_t stdWidth = font->getStdWidth();
    uint8_t stdHeight = font->getStdHeight();
    uint8_t space = font->getSpaceWidth();
    space = GFX_MAX(space / 4, 1);

    while(len--) {
        if(x > clipX2) return;
        uint16_t unicode = str[0] | (str[1] << 8);
        const CharInfo *c = font->getChar(unicode);
        if(c != NULL) {
            DrawChar((Rgb565GfxHelper *)this, c, color, x, y);
            x += c->getWidth() + space;
        }
        else {
            fillRect(color, x, y, stdWidth - 1, stdHeight - 1);
            x += stdWidth + space;
        }
        str += 2;
    }
}
