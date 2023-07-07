#include "texture.h"

Texture::Texture()
{
    image = new TGAImage();
}

Texture::Texture(const char *fileName)
{
    image = new TGAImage();
    image->read_tga_file(fileName);
    width = image->get_width();
    height = image->get_height();
}

Texture::~Texture()
{
    delete image;
}

TGAImage Texture::getImage()
{
    return *image;
}

TGAColor Texture::uv(Vec2f _uv)
{
    float u = _uv.x, v = _uv.y;
    return uv(u, v);
}

TGAColor Texture::uv(float u, float v)
{
    int x = u * width + .5, y = v * height + .5;
    return image->get(x, height - y - 1);
}
