#include <algorithm>
#include <ctime>
#include "tgaimage.h"
#include "geometry.h"
#include "model.h"

const int width = 800;
const int height = 800;
const TGAColor white(255, 255, 255, 255);
const TGAColor red(255, 0, 0, 255);
Model *model;
Vec3f light_dir(0, 0, -1);
float *zbuffer;
int getIndex(int x, int y)
{
    return width * y + x;
}
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

// 扫线
//  void triangle(Vec2i *t, TGAImage &image, TGAColor color)
//  {
//      std::sort(t, t + 2, [](Vec2i u, Vec2i v)
//                { return u.y < v.y; });
//      int totalHeight = t[2].y - t[0].y;
//      for (int y = t[0].y; y < t[1].y; y++)
//      {
//          int segmentHeight = t[1].y - t[0].y;
//          float alpha = (float)(y - t[0].y) / totalHeight;
//          float beta = (float)(y - t[0].y) / segmentHeight;
//          Vec2i A = t[0] + (t[2] - t[0]) * alpha;
//          Vec2i B = t[0] + (t[1] - t[0]) * beta;
//          line(A.x, y, B.x, y, image, color);
//      }
//  }

Vec3f barycentric(Vec3f A, Vec3f B, Vec3f C, Vec3f P)
{
    Vec3f AB = B - A, AC = C - A, PA = A - P;
    Vec3f u = Vec3f(AB.x, AC.x, PA.x), v = Vec3f(AB.y, AC.y, PA.y);
    Vec3f UV = cross(u, v);
    if (std::fabs(UV.z) > 1e-2)
        return Vec3f((1.f - (UV.x + UV.y) / UV.z), UV.y / UV.z, UV.x / UV.z);
    return Vec3f(1, 1, -1);
}

// 重心坐标
void triangle(Vec3f *pts, float *zbuffer, TGAImage &image, TGAColor color)
{
    // 包围盒
    Vec2f bbomin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f bbomax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    Vec2f clamp(image.get_width() - 1, image.get_height() - 1);
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            bbomin[j] = std::max(0.f, std::min(bbomin[j], pts[i][j]));
            bbomax[j] = std::min(clamp[j], std::max(bbomax[j], pts[i][j]));
        }
    }
    Vec3f P;
    for (P.x = bbomin.x; P.x <= bbomax.x; P.x++)
    {
        for (P.y = bbomin.y; P.y <= bbomax.y; P.y++)
        {
            Vec3f bc_screen = barycentric(pts[0], pts[1], pts[2], P);
            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
                continue;
            P.z = 0;
            for (int i = 0; i < 3; i++)
                P.z += pts[i].z * bc_screen[i];
            if (zbuffer[getIndex(P.x, P.y)] < P.z)
            {
                zbuffer[getIndex(P.x, P.y)] = P.z;
                image.set(P.x, P.y, color);
            }
        }
    }
}

Vec3f world2screen(Vec3f v)
{
    return Vec3f(int((v.x + 1.) * width / 2. + .5), int((v.y + 1.) * height / 2. + .5), v.z);
}

int main(int argc, char **argv)
{
    srand(time(0));
    TGAImage render(width, height, TGAImage::RGB);
    zbuffer = new float[width * height];
    for (int i = 0; i < width * height; i++)
        zbuffer[i] = -std::numeric_limits<float>::max();
    model = new Model("obj\\african_head\\african_head.obj");
    for (int i = 0; i < model->nfaces(); i++)
    {
        std::vector<int> f = model->face(i);
        Vec3f pts[3], worldCoords[3];
        for (int j = 0; j < 3; j++)
        {
            worldCoords[j] = model->vert(f[j]);
            pts[j] = world2screen(model->vert(f[j]));
        }

        // 计算法线
        Vec3f n = cross(worldCoords[2] - worldCoords[0], worldCoords[1] - worldCoords[0]);
        n.normalize();
        float intensity = n * light_dir;
        std::cerr << n << std::endl;
        if (intensity > 0)
            triangle(pts, zbuffer, render, TGAColor(255 * intensity, 255 * intensity, 255 * intensity, 255));
    }
    render.flip_vertically();
    render.write_tga_file("test.tga");
    delete model;
    return 0;
}