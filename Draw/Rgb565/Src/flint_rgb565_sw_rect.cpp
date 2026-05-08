
#include "flint_rgb565_draw_rect.h"

static void Rgb565_DrawHLine(Gfx *g, uint32_t thickness, uint32_t color, int32_t x1, int32_t x2, int32_t y) {
    x1 = F_X1(g, x1);
    x2 = F_X2(g, x2);
    uint8_t alpha = color >> 27;
    if(thickness > 1) {
        int32_t half = (thickness - 1) >> 1;
        int32_t y1 = F_Y1(g, y - half);
        int32_t y2 = F_Y2(g, y + half);
        if(alpha == 0x1F) {
            for(; y1 <= y2; y1++)
                SET_H_LINE(g, color, x1, x2, y1);
        }
        else {
            uint32_t fg = __builtin_bswap16(color);
            fg = (fg | (fg << 16)) & 0x07E0F81F;
            for(; y1 <= y2; y1++)
                BLEND_H_LINE(g, alpha, fg, x1, x2, y1);
        }
    }
    else if(IN_Y_CLIP(g, y)) {
        if(alpha == 0x1F)
            SET_H_LINE(g, color, x1, x2, y);
        else {
            uint32_t fg = __builtin_bswap16(color);
            fg = (fg | (fg << 16)) & 0x07E0F81F;
            BLEND_H_LINE(g, alpha, fg, x1, x2, y);
        }
    }
}

static void Rgb565_DrawVLine(Gfx *g, uint32_t thickness, uint32_t color, int32_t y1, int32_t y2, int32_t x) {
    y1 = F_Y1(g, y1);
    y2 = F_Y2(g, y2);
    uint8_t alpha = color >> 27;
    if(thickness > 1) {
        int32_t half = (thickness - 1) >> 1;
        int32_t x1 = F_X1(g, x - half);
        int32_t x2 = F_X2(g, x + half);
        if(alpha == 0x1F) {
            for(; x1 <= x2; x1++)
                SET_V_LINE(g, color, y1, y2, x1);
        }
        else {
            uint32_t fg = __builtin_bswap16(color);
            fg = (fg | (fg << 16)) & 0x07E0F81F;
            for(; x1 <= x2; x1++)
                BLEND_V_LINE(g, alpha, fg, y1, y2, x1);
        }
    }
    else if(IN_X_CLIP(g, x)) {
        if(alpha == 0x1F)
            SET_V_LINE(g, color, y1, y2, x);
        else {
            uint32_t fg = __builtin_bswap16(color);
            fg = (fg | (fg << 16)) & 0x07E0F81F;
            BLEND_V_LINE(g, alpha, fg, y1, y2, x);
        }
    }
}

void Rgb565_DrawRect(Gfx *g, uint32_t color, uint32_t thickness, int32_t x, int32_t y, int32_t w, int32_t h) {
    int32_t half = (thickness - 1) >> 1;

    int32_t x1 = x;
    int32_t x2 = x + w;
    int32_t y1 = y;
    int32_t y2 = y + h;

    if((thickness & 0x01) == 0) {
        uint32_t c = ((color >> 1) & 0xFF000000) | (color & 0xFFFF);

        Rgb565_DrawHLine(g, 1, c, x1 - half - 1, x2 + half + 1, y1 - half - 1);
        Rgb565_DrawHLine(g, 1, c, x1 - half - 1, x2 + half + 1, y2 + half + 1);

        Rgb565_DrawHLine(g, 1, c, x1 + half + 1, x2 - half - 1, y1 + half + 1);
        Rgb565_DrawHLine(g, 1, c, x1 + half + 1, x2 - half - 1, y2 - half - 1);

        Rgb565_DrawVLine(g, 1, c, y1 - half, y2 + half, x1 - half - 1);
        Rgb565_DrawVLine(g, 1, c, y1 - half, y2 + half, x2 + half + 1);

        Rgb565_DrawVLine(g, 1, c, y1 + half + 1, y2 - half - 1, x1 + half + 1);
        Rgb565_DrawVLine(g, 1, c, y1 + half + 1, y2 - half - 1, x2 - half - 1);

        thickness--;
    }
    Rgb565_DrawHLine(g, thickness, color, x1 - half, x2 + half, y1);
    Rgb565_DrawHLine(g, thickness, color, x1 - half, x2 + half, y2);

    Rgb565_DrawVLine(g, thickness, color, y1 + half + 1, y2 - half - 1, x1);
    Rgb565_DrawVLine(g, thickness, color, y1 + half + 1, y2 - half - 1, x2);
}

void Rgb565_FillRect(Gfx *g, uint32_t color, int32_t x, int32_t y, int32_t w, int32_t h) {
    int32_t x1 = F_X1(g, x);
    int32_t x2 = F_X2(g, x + w);
    int32_t y1 = F_Y1(g, y);
    int32_t y2 = F_Y2(g, y + h);

    Rgb565_Fill(g, color, x1, y1, x2, y2);
}
