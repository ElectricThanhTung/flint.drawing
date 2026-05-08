
#include <math.h>
#include "flint_rgb565_draw_ellipse.h"

static constexpr uint16_t fpmax = (1 << FRACTIONAL_BITS);
static constexpr uint16_t fphalf = (1 << (FRACTIONAL_BITS - 1));
static constexpr uint16_t fpmask = fpmax - 1;

static uint64_t ISqrt(uint64_t x) {
    uint64_t g, ret = 1ull << ((63 - __builtin_clzll(x)) >> 1);
    do {
        g = ret;
        ret = (g + x / g) >> 1;
    } while((ret >> 1) != (g >> 1));
    return ret;
}

static void Rgb565_SetFillEllipse(Gfx *g, uint32_t color, int32_t x, int32_t y, uint32_t w, uint32_t h) {
    uint32_t fg = __builtin_bswap16(color);
    fg = (fg | (fg << 16)) & 0x07E0F81F;

    int32_t cx = x + w / 2;
    int32_t cy = y + h / 2;

    uint64_t ww4 = ((uint64_t)w * w) << 2;
    uint64_t hh4 = ((uint64_t)h * h) << 2;
    uint64_t wh2 = (uint64_t)w * w * h * h;

    if(IN_Y_CLIP(g, cy)) {
        if(w & 0x01)
            SET_H_LINE(g, color, F_X1(g, x), F_X2(g, x + w - 1), cy);
        else {
            SET_H_LINE(g, color, F_X1(g, x + 1), F_X2(g, x + w -1), cy);
            if(IN_X_CLIP(g, x)) BLEND_PIXEL(PIXEL(g, x, cy), 16, fg);
            if(IN_X_CLIP(g, x + w)) BLEND_PIXEL(PIXEL(g, x + w, cy), 16, fg);
        }
    }
    int32_t ix = w, iy = 1;
    uint16_t off = (w & 0x01) ? 0 : fphalf;
    for(; hh4 * ix >= ww4 * iy; iy++) {
        int64_t fx = ISqrt(((wh2 - ww4 * iy * iy) / hh4) << (FRACTIONAL_BITS << 1)) - off;
        ix = (int32_t)(fx >> FRACTIONAL_BITS);
        uint8_t al = (uint8_t)(((uint32_t)fx & fpmask) * 31 / fpmax);
        int32_t x1 = cx - ix;
        int32_t x2 = cx + ix;
        if(IN_Y_CLIP(g, cy - iy)) SET_H_LINE(g, color, F_X1(g, x1), F_X2(g, x2), cy - iy);
        if(IN_Y_CLIP(g, cy + iy)) SET_H_LINE(g, color, F_X1(g, x1), F_X2(g, x2), cy + iy);
        if(al > 0) {
            if(IN_CLIP(g, x1 - 1, cy - iy)) BLEND_PIXEL(PIXEL(g, x1 - 1, cy - iy), al, fg);
            if(IN_CLIP(g, x2 + 1, cy - iy)) BLEND_PIXEL(PIXEL(g, x2 + 1, cy - iy), al, fg);
            if(IN_CLIP(g, x1 - 1, cy + iy)) BLEND_PIXEL(PIXEL(g, x1 - 1, cy + iy), al, fg);
            if(IN_CLIP(g, x2 + 1, cy + iy)) BLEND_PIXEL(PIXEL(g, x2 + 1, cy + iy), al, fg);
        }
    }
    off = (h & 0x01) ? 0 : fphalf;
    for(ix; ix > 0; ix--) {
        int64_t fy = ISqrt(((wh2 - hh4 * ix * ix) / ww4) << (FRACTIONAL_BITS << 1)) - off;
        int32_t i = (int32_t)(fy >> FRACTIONAL_BITS);
        uint8_t al = (uint8_t)(((uint32_t)fy & fpmask) * 31 / fpmax);
        int32_t y1 = cy - i;
        int32_t y2 = cy + i;
        if(IN_X_CLIP(g, cx - ix)) {
            SET_V_LINE(g, color, F_Y1(g, y1), F_Y2(g, cy - iy), cx - ix);
            SET_V_LINE(g, color, F_Y1(g, cy + iy), F_Y2(g, y2), cx - ix);
        }
        if(IN_X_CLIP(g, cx + ix)) {
            SET_V_LINE(g, color, F_Y1(g, y1), F_Y2(g, cy - iy), cx + ix);
            SET_V_LINE(g, color, F_Y1(g, cy + iy), F_Y2(g, y2), cx + ix);
        }
        if(al > 0) {
            if(IN_CLIP(g, cx - ix, y1 - 1)) BLEND_PIXEL(PIXEL(g, cx - ix, y1 - 1), al, fg);
            if(IN_CLIP(g, cx + ix, y1 - 1)) BLEND_PIXEL(PIXEL(g, cx + ix, y1 - 1), al, fg);
            if(IN_CLIP(g, cx - ix, y2 + 1)) BLEND_PIXEL(PIXEL(g, cx - ix, y2 + 1), al, fg);
            if(IN_CLIP(g, cx + ix, y2 + 1)) BLEND_PIXEL(PIXEL(g, cx + ix, y2 + 1), al, fg);
        }
    }
    if(IN_X_CLIP(g, cx)) {
        if(h & 0x01) {
            SET_V_LINE(g, color, F_Y1(g, y), F_Y2(g, cy - iy), cx);
            SET_V_LINE(g, color, F_Y1(g, cy + iy), F_Y2(g, y + h - 1), cx);
        }
        else {
            SET_V_LINE(g, color, F_Y1(g, y + 1), F_Y2(g, cy - iy), cx);
            SET_V_LINE(g, color, F_Y1(g, cy + iy), F_Y2(g, y + h - 1), cx);
            if(IN_Y_CLIP(g, y)) BLEND_PIXEL(PIXEL(g, cx, y), 16, fg);
            if(IN_Y_CLIP(g, y + h)) BLEND_PIXEL(PIXEL(g, cx, y + h), 16, fg);
        }
    }
}

static void Rgb565_BlendFillEllipse(Gfx *g, uint32_t color, int32_t x, int32_t y, uint32_t w, uint32_t h) {
    uint8_t alpha = color >> 27;
    uint32_t fg = __builtin_bswap16(color);
    fg = (fg | (fg << 16)) & 0x07E0F81F;

    int32_t cx = x + w / 2;
    int32_t cy = y + h / 2;

    uint64_t ww4 = ((uint64_t)w * w) << 2;
    uint64_t hh4 = ((uint64_t)h * h) << 2;
    uint64_t wh2 = (uint64_t)w * w * h * h;

    if(IN_Y_CLIP(g, cy)) {
        if(w & 0x01)
            BLEND_H_LINE(g, alpha, fg, F_X1(g, x), F_X2(g, x + w - 1), cy);
        else {
            uint8_t al = (alpha + 1) / 2;
            BLEND_H_LINE(g, alpha, fg, F_X1(g, x + 1), F_X2(g, x + w -1), cy);
            if(IN_X_CLIP(g, x)) BLEND_PIXEL(PIXEL(g, x, cy), al, fg);
            if(IN_X_CLIP(g, x + w)) BLEND_PIXEL(PIXEL(g, x + w, cy), al, fg);
        }
    }
    int32_t ix = w, iy = 1;
    uint16_t off = (w & 0x01) ? 0 : fphalf;
    for(; hh4 * ix >= ww4 * iy; iy++) {
        int64_t fx = ISqrt(((wh2 - ww4 * iy * iy) / hh4) << (FRACTIONAL_BITS << 1)) - off;
        ix = (int32_t)(fx >> FRACTIONAL_BITS);
        uint8_t al = (uint8_t)(((uint32_t)fx & fpmask) * alpha / fpmax);
        int32_t x1 = cx - ix;
        int32_t x2 = cx + ix;
        if(IN_Y_CLIP(g, cy - iy)) BLEND_H_LINE(g, alpha, fg, F_X1(g, x1), F_X2(g, x2), cy - iy);
        if(IN_Y_CLIP(g, cy + iy)) BLEND_H_LINE(g, alpha, fg, F_X1(g, x1), F_X2(g, x2), cy + iy);
        if(al > 0) {
            if(IN_CLIP(g, x1 - 1, cy - iy)) BLEND_PIXEL(PIXEL(g, x1 - 1, cy - iy), al, fg);
            if(IN_CLIP(g, x2 + 1, cy - iy)) BLEND_PIXEL(PIXEL(g, x2 + 1, cy - iy), al, fg);
            if(IN_CLIP(g, x1 - 1, cy + iy)) BLEND_PIXEL(PIXEL(g, x1 - 1, cy + iy), al, fg);
            if(IN_CLIP(g, x2 + 1, cy + iy)) BLEND_PIXEL(PIXEL(g, x2 + 1, cy + iy), al, fg);
        }
    }
    off = (h & 0x01) ? 0 : fphalf;
    for(ix; ix > 0; ix--) {
        int64_t fy = ISqrt(((wh2 - hh4 * ix * ix) / ww4) << (FRACTIONAL_BITS << 1)) - off;
        int32_t i = (int32_t)(fy >> FRACTIONAL_BITS);
        uint8_t al = (uint8_t)(((uint32_t)fy & fpmask) * alpha / fpmax);
        int32_t y1 = cy - i;
        int32_t y2 = cy + i;
        if(IN_X_CLIP(g, cx - ix)) {
            BLEND_V_LINE(g, alpha, fg, F_Y1(g, y1), F_Y2(g, cy - iy), cx - ix);
            BLEND_V_LINE(g, alpha, fg, F_Y1(g, cy + iy), F_Y2(g, y2), cx - ix);
        }
        if(IN_X_CLIP(g, cx + ix)) {
            BLEND_V_LINE(g, alpha, fg, F_Y1(g, y1), F_Y2(g, cy - iy), cx + ix);
            BLEND_V_LINE(g, alpha, fg, F_Y1(g, cy + iy), F_Y2(g, y2), cx + ix);
        }
        if(al > 0) {
            if(IN_CLIP(g, cx - ix, y1 - 1)) BLEND_PIXEL(PIXEL(g, cx - ix, y1 - 1), al, fg);
            if(IN_CLIP(g, cx + ix, y1 - 1)) BLEND_PIXEL(PIXEL(g, cx + ix, y1 - 1), al, fg);
            if(IN_CLIP(g, cx - ix, y2 + 1)) BLEND_PIXEL(PIXEL(g, cx - ix, y2 + 1), al, fg);
            if(IN_CLIP(g, cx + ix, y2 + 1)) BLEND_PIXEL(PIXEL(g, cx + ix, y2 + 1), al, fg);
        }
    }
    if(IN_X_CLIP(g, cx)) {
        if(h & 0x01) {
            BLEND_V_LINE(g, alpha, fg, F_Y1(g, y), F_Y2(g, cy - iy), cx);
            BLEND_V_LINE(g, alpha, fg, F_Y1(g, cy + iy), F_Y2(g, y + h - 1), cx);
        }
        else {
            uint8_t al = (alpha + 1) / 2;
            BLEND_V_LINE(g, alpha, fg, F_Y1(g, y + 1), F_Y2(g, cy - iy), cx);
            BLEND_V_LINE(g, alpha, fg, F_Y1(g, cy + iy), F_Y2(g, y + h - 1), cx);
            if(IN_Y_CLIP(g, y)) BLEND_PIXEL(PIXEL(g, cx, y), al, fg);
            if(IN_Y_CLIP(g, y + h)) BLEND_PIXEL(PIXEL(g, cx, y + h), al, fg);
        }
    }
}

void Rgb565_FillEllipse(Gfx *g, uint32_t color, int32_t x, int32_t y, uint32_t w, uint32_t h) {
    uint8_t alpha = color >> 27;
    if(alpha == 0x1F)
        Rgb565_SetFillEllipse(g, color, x, y, w, h);
    else
        Rgb565_BlendFillEllipse(g, color, x, y, w, h);
}
