
#ifndef __FLINT_GFX_H
#define __FLINT_GFX_H

#include <stdint.h>

#define FRACTIONAL_BITS                                                 4

#define F_MAX(_a, _b)                                                   ((_a) > (_b) ? (_a) : (_b))
#define F_MIN(_a, _b)                                                   ((_a) < (_b) ? (_a) : (_b))
#define F_ABS(_a)                                                       ((_a) < 0 ? -(_a) : (_a)) 
#define F_SWAP(_a, _b)                                                  do { decltype(_a) tmp = (_a); (_a) = (_b); (_b) = tmp; } while(0)

#define F_X1(_gfx, _x)                                                  F_MAX((_gfx)->clipX1, _x)
#define F_X2(_gfx, _x)                                                  F_MIN((_gfx)->clipX2, _x)
#define F_Y1(_gfx, _y)                                                  F_MAX((_gfx)->clipY1, _y)
#define F_Y2(_gfx, _y)                                                  F_MIN((_gfx)->clipY2, _y)

#define IN_X_CLIP(_gfx, _x)                                             ((_x) >= (_gfx)->clipX1 && (_x) <= (_gfx)->clipX2)
#define IN_Y_CLIP(_gfx, _y)                                             ((_y) >= (_gfx)->clipY1 && (_y) <= (_gfx)->clipY2)
#define IN_CLIP(_gfx, _x, _y)                                           (IN_X_CLIP((_gfx), (_x)) && IN_Y_CLIP((_gfx), (_y)))

class Gfx {
public:
    int32_t w;
    int32_t h;
    int32_t clipX1;
    int32_t clipY1;
    int32_t clipX2;
    int32_t clipY2;
    uint8_t *data;
};

#endif /* __FLINT_GFX_H */
