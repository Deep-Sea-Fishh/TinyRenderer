#ifndef __MYGL_H__
#define __MYGL_H__

#include "geometry.h"
#include "tgaimage.h"

Matrix getModel(Vec3f scale);
Matrix getView(Vec3f pos, Vec3f center, Vec3f up);
Matrix getProjection(float near, float far, float fov, float aspect);
Matrix getViewport(int width, int height);

struct IShader
{
    virtual ~IShader();
    virtual Vec4f vertex(int iface, int nthvert) = 0;
    virtual bool fragment(Vec3f bar, TGAColor &color) = 0;
};

void triangle(Vec4f *pts, IShader &shader, TGAImage &image, float *zbuffer);

#endif