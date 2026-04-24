
#include <string.h>
#include <windows.h>
#include <stdlib.h>
#include <stdint.h>
#include <graphics.h>

#include "flint_rgb565_draw_line.h"
#include "flint_rgb565_draw_rect.h"
#include "flint_rgb565_draw_ellipse.h"

#include <math.h>

using namespace std;

static void WriteScreen(uint8_t *rgbBuff, uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    uint32_t sz = imagesize(x, y, x + width - 1, y + height - 1);
    int32_t *image = (int32_t *)malloc(sz);
    uint16_t *buff = (uint16_t *)rgbBuff;
    getimage(x, y, x + width - 1, y + height - 1, image);
    for(uint32_t i = 6; i < (sz / 4); i++) {
        // image[i] = __builtin_bswap16(buff[i - 6]);
        uint16_t c = __builtin_bswap16(buff[i - 6]);
        uint8_t r = (c >> 11) * 0xFF / 0x1F;
        uint8_t g = ((c >> 5) & 0x3F) * 0xFF / 0x3F;
        uint8_t b = (c & 0x1F) * 0xFF / 0x1F;
        image[i] = (r << 16) | (g << 8) | b;
    }
    putimage(x, y, image, COPY_PUT);
    free(image);
}

uint32_t Color(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
    uint32_t tmp = (r << 8) & 0xF800;
    tmp |= (g << 3) & 0x07E0;
    tmp |= (b >> 3) & 0x1F;
    tmp = __builtin_bswap16(tmp);
    tmp |= a << 24;
    return tmp;
}

int main(void) {
    int gd = DETECT, gm;
    initgraph(&gd, &gm, (char *)"");

    int w = 500;
    int h = 500;

    uint8_t *buff = new uint8_t[w * h * 2];
    memset(buff, 0, w * h * 2);
    FGfx g;
    g.w = w;
    g.h = h;
    g.clipX1 = 0;
    g.clipY1 = 0;
    g.clipX2 = w - 1;
    g.clipY2 = h - 1;
    g.data = buff;

    uint8_t alpha = 0xFF;
    uint32_t off = 300;

    // Rgb565_DrawLine(&g, Color(alpha, 0, 255, 0), 100, off, off, off + 10, off + 100);

    // Rgb565_FillRect(&g, Color(alpha, 0, 255, 0), 10, 10, 100, 200);

    Rgb565_FillEllipse(&g, Color(alpha, 0, 255, 0), 10, 10, 100, 50);

    WriteScreen(buff, 0, 0, w, h);

    getch();
    closegraph();

    return 0;
}
