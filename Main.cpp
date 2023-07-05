#include "tgaimage.h"
#include "geometry.h"
#include "model.h"

const int width = 800;
const int height = 800;
const TGAColor white(255, 255, 255, 255);
const TGAColor red(255, 0, 0, 255);

Model *model;
void line(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color)
{
    bool steep = false;
    if (std::abs(x1 - x0) < std::abs(y1 - y0))
    {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    if (x0 > x1)
    {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    int dx = x1 - x0;
    int dy = y1 - y0;
    int y = y0;
    int error = 0;
    // float derror = std::abs(dy / float(dx));
    int derror = std::abs(dy) * 2;
    for (int x = x0; x <= x1; x++)
    {
        steep ? image.set(y, x, color) : image.set(x, y, color);
        error += derror;
        if (error > dx)
        {
            y += y1 > y0 ? 1 : -1;
            error -= 2 * dx;
        }
    }
}

int main(int argc, char **argv)
{
    TGAImage image(width, height, TGAImage::RGB);

    if (argc > 1)
    {
        std::cout << argv[1] << std::endl;
        model = new Model(argv[1]);
    }
    else
        model = new Model("obj\\african_head\\african_head.obj");
    for (int i = 0; i < model->nfaces(); i++)
    {
        std::vector<int> f = model->face(i);
        for (int j = 0; j < 3; j++)
        {
            Vec3f u = model->vert(f[j]), v = model->vert(f[(j + 1) % 3]);
            int x0 = (u.x + 1.) * width / 2.;
            int y0 = (u.y + 1.) * height / 2.;
            int x1 = (v.x + 1.) * width / 2.;
            int y1 = (v.y + 1.) * height / 2.;
            line(x0, y0, x1, y1, image, white);
        }
    }

    image.flip_vertically();
    image.write_tga_file("test.tga");
    delete model;
    return 0;
}