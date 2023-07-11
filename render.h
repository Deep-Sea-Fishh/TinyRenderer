#ifndef __RENDER_H__
#define __RENDER_H__

#include "geometry.h"
#include "tgaimage.h"

extern Matrix ModelView;
extern Matrix Projection;
extern Matrix Viewport;

void getView(Vec3f pos, Vec3f center, Vec3f up);
void getProjection(float near, float far, float fov, float aspect);
void getViewport(int width, int height);
struct IShader
{
    virtual ~IShader();
    virtual Vec4f vertex(int iface, int nthvert) = 0;
    virtual bool fragment(Vec3f bar, TGAColor &color) = 0;
};

enum MSAA
{
    ONE_ONE = 1,
    TWO_TWO
};

class Render
{
private:
    int width;
    int height;
    IShader *shader;
    TGAImage *image;
    float *zbuffer;
    MSAA msaa;
    TGAImage *superImage;
    float *superZbuffer;

public:
    Render(int width, int height, IShader *shader, MSAA msaa);
    ~Render();
    void triangle(mat<4, 3, float> &clipc);
    int getWidth();
    int getHeight();
    int getIndex(int x, int y) { return y * width + x; }
    int getSuperIndex(int x, int y) { return y * width * msaa + x; }

    TGAImage *getImage() { return image; }
    TGAImage *getSuperImage() { return superImage; }
};

#endif