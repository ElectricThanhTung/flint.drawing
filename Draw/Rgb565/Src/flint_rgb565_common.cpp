
#include "flint_rgb565_common.h"

void Rgb565_Fill(FGfx *g, uint32_t color, int32_t x1, uint32_t y1, int32_t x2, int32_t y2) {
    uint8_t alpha = color >> 27;
    if(alpha == 0x1F) {
        uint16_t c = color;
        if(x1 == x2)
            SET_V_LINE(g, color, y1, y2, x1);
        else for(uint32_t y = y1; y <= y2; y++)
            SET_H_LINE(g, color, x1, x2, y);
    }
    else {
        uint32_t fg = __builtin_bswap16(color);
        fg = (fg | (fg << 16)) & 0x07E0F81F;
        if(x1 == x2)
            BLEND_V_LINE(g, alpha, fg, y1, y2, x1);
        else for(uint32_t y = y1; y <= y2; y++)
            BLEND_H_LINE(g, alpha, fg, x1, x2, y);
    }
}
