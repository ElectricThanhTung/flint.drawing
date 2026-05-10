
#include <string.h>
#include <windows.h>
#include <stdlib.h>
#include <stdint.h>
#include <graphics.h>

#include "flint_rgb565_gfx.h"

#include <math.h>

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

class Rgb565GfxInitHelper : public Rgb565Gfx {
public:
    Rgb565GfxInitHelper(int32_t w, int32_t h, uint8_t *data) : Rgb565Gfx(
        w, h, 0, 0, w - 1, h - 1, data
    ){

    }
};

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
    Rgb565GfxInitHelper g(w, h, (uint8_t *)buff);

    // g.drawLine(Color(255, 255, 255, 255), 2, 10, 10, 100, 10);
    // g.drawLine(Color(255, 255, 255, 255), 10, 10, 10, 100, 20);

    // g.fillRect(Color(255, 255, 255, 255), 10, 10, 100, 50);

    // g.fillEllipse(Color(127, 255, 255, 255), 10, 10, 100, 400);

    // g.drawLatin1((uint8_t *)"Hello Thanh Tùng", 17, (Font *)Times_New_Roman, Color(127, 255, 255, 255), 10, 10);

    g.drawEllipse(Color(128, 0, 255, 0), 1, 50, 50, 201, 200);

    WriteScreen((uint8_t *)buff, 0, 0, w, h);

    getch();
    closegraph();

    return 0;
}
