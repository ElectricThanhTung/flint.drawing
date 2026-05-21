
#include <string.h>
#include <windows.h>
#include <stdlib.h>
#include <stdint.h>
#include <graphics.h>
#include "flint_rgb565_gfx.h"

using namespace std;

static void WriteScreen(uint8_t *rgbBuff, uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    uint32_t sz = imagesize(x, y, x + width - 1, y + height - 1);
    int32_t *image = (int32_t *)malloc(sz);
    uint16_t *buff = (uint16_t *)rgbBuff;
    getimage(x, y, x + width - 1, y + height - 1, image);
    for(uint32_t i = 6; i < (sz / 4); i++) {
        // image[i] = __builtin_bswap16(buff[i - 6]);
        uint16_t c = buff[i - 6];
        uint8_t r = ((c >> 8) & 0x1F) * 0xFF / 0x1F;
        uint8_t g = (((c << 3) & 0x38) | (c >> 13)) * 0xFF / 0x3F;
        uint8_t b = ((c >> 3) & 0x1F) * 0xFF / 0x1F;
        image[i] = (r << 16) | (g << 8) | b;
    }
    putimage(x, y, image, COPY_PUT);
    free(image);
}

uint32_t Color(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
    uint32_t tmp = (r << 5) & 0x1F00;
    tmp |= b & 0xF8;
    tmp |= ((g >> 5) | (g << 13)) & 0xE007;
    tmp |= a << 24;
    return tmp;
}

int main(void) {
    int gd = DETECT, gm;
    initgraph(&gd, &gm, (char *)"");

    int w = 500;
    int h = 500;

    uint16_t *buff = new uint16_t[w * h];
    memset(buff, 0, w * h * 2);
    Rgb565Gfx g(
        w, h,
        0, 0,
        499, 499,
        (uint8_t *)buff
    );

    // g.drawRect(Color(255, 255, 0, 0), 2, 10, 10, 201, 101);
    // g.fillEllipse(Color(128, 0, 255, 0), 10, 10, 200, 200);
    // g.fillRoundRect(Color(128, 0, 255, 0), 10, 100 - 160, 200, 100, 80, 10, 10, 10);

    // g.drawRoundRect(Color(128, 0, 255, 0), 6, 50, 50, 201, 201, 20, 20, 30, 20);

    g.drawEllipse(Color(128, 0, 255, 0), 6, 10, 10, 39, 39);

    g.fillEllipse(Color(128, 0, 255, 0), 100, 100, 45, 45);

    WriteScreen((uint8_t *)buff, 0, 0, w, h);

    getch();
    closegraph();

    return 0;
}
