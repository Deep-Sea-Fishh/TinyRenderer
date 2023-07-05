#include <algorithm>
#include <ctime>
#include "tgaimage.h"
#include "geometry.h"
#include "model.h"

const int width = 800;
const int height = 800;
const TGAColor white(255, 255, 255, 255);
const TGAColor red(255, 0, 0, 255);
Vec2i t[3] = {{100, 100}, {500, 300}, {300, 500}};
Model *model;
Vec3f light_dir(0, 0, -1);
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

Vec3f barycentric(int x, int y, Vec2i *t)
{
    Vec2i p(x, y);
    Vec2i AB = t[1] - t[0], AC = t[2] - t[0], PA = t[0] - p;
    Vec3f u(AB.x, AC.x, PA.x), v(AB.y, AC.y, PA.y);
    Vec3f uv = u ^ v;
    if (uv.z < 0)
        return {1, 1, -1};
    return Vec3f(1. - (uv.x + uv.y) / uv.z, uv.y / uv.z, uv.x / uv.z);
}
bool inside(int x, int y, Vec2i *t)
{
    Vec3f tuple = barycentric(x, y, t);
    if (tuple.x < 0 || tuple.y < 0 || tuple.z < 0)
        return false;
    return true;
}

// 重心坐标
void triangle(Vec2i *t, TGAImage &image, TGAColor color)
{
    // 找包围框
    int min_x = width, max_x = 0, min_y = height, max_y = 0;
    for (int i = 0; i < 3; i++)
    {
        min_x = std::min(min_x, t[i].x);
        max_x = std::max(max_x, t[i].x);
        min_y = std::min(min_y, t[i].y);
        max_y = std::max(max_y, t[i].y);
    }
    for (int x = min_x; x <= max_x; x++)
    {
        for (int y = min_y; y <= max_y; y++)
        {
            if (inside(x, y, t))
            {
                image.set(x, y, color);
            }
        }
    }
}

int main(int argc, char **argv)
{
    srand(time(0));
    TGAImage image(width, height, TGAImage::RGB);
    model = new Model("obj\\african_head\\african_head.obj");
    for (int i = 0; i < model->nfaces(); i++)
    {
        std::vector<int> f = model->face(i);
        Vec2i screenCoords[3];
        Vec3f worldCoords[3];
        for (int j = 0; j < 3; j++)
        {
            screenCoords[j] = Vec2i((model->vert(f[j]).x + 1.) * width / 2., (model->vert(f[j]).y + 1.) * height / 2.);
            worldCoords[j] = model->vert(f[j]);
        }
        // 计算法线
        Vec3f n = (worldCoords[2] - worldCoords[0]) ^ (worldCoords[1] - worldCoords[0]);
        // 计算光照
        n.normlize();
        float intensity = light_dir * n;
        triangle(screenCoords, image, TGAColor(255 * intensity, 255 * intensity, 255 * intensity, 255));
    }
    image.flip_vertically();
    image.write_tga_file("test.tga");
    delete model;
    return 0;
}