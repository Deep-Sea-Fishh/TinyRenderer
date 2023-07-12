#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include <fstream>
#include "tgaimage.h"
#include "geometry.h"
class Texture
{
private:
    TGAImage *image;
    int width, height;

public:
    Texture();
    Texture(const char *fileName);
    ~Texture();
    TGAImage getImage();
    TGAColor uv(Vec2f _uv);
    TGAColor uv(float u, float v);
};

#endif
