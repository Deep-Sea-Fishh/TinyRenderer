#include <algorithm>
#include <ctime>
#include "tgaimage.h"
#include "geometry.h"
#include "model.h"
#include "texture.h"
#include "myGL.h"

const int width = 800;
const int height = 800;

Model *model;
Texture *texture;
Vec3f light_dir(0, 0, -1);

Matrix ModelView;
Matrix Viewport;
Matrix Projection;

const Vec3f cameraPos(0, 0, 3);
const Vec3f cameraCenter(0, 0, 0);
const Vec3f cameraUp(0, 1, 0);

Vec4f m2v(mat<4, 1, float> m)
{
    int rd = rand();
    Vec4f res;
    res[0] = m[0][0] / m[3][0];
    res[1] = m[1][0] / m[3][0];
    res[2] = m[2][0] / m[3][0];
    res[3] = 1;

    return res;
}
mat<4, 1, float> v2m(Vec3f v)
{

    mat<4, 1, float> mat;
    mat[0][0] = v[0];
    mat[1][0] = v[1];
    mat[2][0] = v[2];
    mat[3][0] = 1.f;
    return mat;
}

struct GouraudShader : public IShader
{
    Vec3f varying_intensity; // written by vertex shader, read by fragment shader

    virtual Vec4f vertex(int iface, int nthvert)
    {
        Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert));                                  // read the vertex from .obj file
        gl_Vertex = Viewport * Projection * ModelView * gl_Vertex;                                // transform it to screen coordinates
        varying_intensity[nthvert] = std::max(0.f, -(model->normal(iface, nthvert) * light_dir)); // get diffuse lighting intensity
        Vec3f tem = model->normal(iface, nthvert);
        return gl_Vertex;
    }

    virtual bool fragment(Vec3f bar, TGAColor &color)
    {
        int rd = rand();
        float intensity = varying_intensity * bar;   // interpolate intensity for the current pixel
        color = TGAColor(255, 255, 255) * intensity; // well duh
        // if (rd % 10 == 0)
        //     std::cout << (int)color[0] << " " << (int)color[1] << " " << (int)color[2] << " " << color[3] << std::endl;
        return false; // no, we do not discard this pixel
    }
};
// int getIndex(int x, int y)
// {
//     return width * y + x;
// }
// Vec3f barycentric(Vec2f A, Vec2f B, Vec2f C, Vec2f P)
// {
//     Vec2f AB = B - A, AC = C - A, PA = A - P;
//     Vec3f u = Vec3f(AB.x, AC.x, PA.x), v = Vec3f(AB.y, AC.y, PA.y);
//     Vec3f UV = cross(u, v);
//     if (std::fabs(UV.z) > 1e-8)
//         return Vec3f((1.f - (UV.x + UV.y) / UV.z), UV.x / UV.z, UV.y / UV.z);
//     return Vec3f(1, 1, -1);
// }

// void triangle(Vec4f *pts, TGAImage &image, Vec3f *normals, TGAColor *textures, float *zbuffer)
// {
//     int rd = rand();
//     // 包围盒
//     Vec2f bbomin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
//     Vec2f bbomax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
//     Vec2f clamp(image.get_width() - 1, image.get_height() - 1);
//     for (int i = 0; i < 3; i++)
//     {
//         for (int j = 0; j < 2; j++)
//         {
//             pts[i][j] = int(pts[i][j] + .5);
//             bbomin[j] = std::max(0.f, std::min(bbomin[j], pts[i][j]));
//             bbomax[j] = std::min(clamp[j], std::max(bbomax[j], pts[i][j]));
//         }
//     }
//     Vec3f P;
//     for (P.x = int(bbomin.x); P.x <= int(bbomax.x + .5); P.x++)
//     {
//         for (P.y = int(bbomin.y); P.y <= int(bbomax.y + .5); P.y++)
//         {
//             Vec3f bc_screen = barycentric(proj<2>(pts[0] / pts[0][3]), proj<2>(pts[1] / pts[1][3]), proj<2>(pts[2] / pts[2][3]), proj<2>(P));
//             if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
//                 continue;
//             P.z = 0;
//             for (int i = 0; i < 3; i++)
//                 P.z += pts[i][2] * bc_screen[i];
//             if (zbuffer[int(P.y * image.get_width() + P.x)] < P.z)
//             {
//                 zbuffer[int(P.y * image.get_width() + P.x)] = P.z;
//                 // 计算法线
//                 Vec3f normal = normals[0] * bc_screen[0] + normals[1] * bc_screen[1] + normals[2] * bc_screen[2];
//                 // std::cout << normal;
//                 //  计算光照
//                 float intensity = -(normal * light_dir);
//                 if (intensity <= 0)
//                     continue;
//                 // 计算颜色
//                 TGAColor color(0, 0, 0, 255);
//                 color.bgra[0] = textures[0][0] * bc_screen[0] + textures[1][0] * bc_screen[1] + textures[2][0] * bc_screen[2];
//                 color.bgra[1] = textures[0][1] * bc_screen[0] + textures[1][1] * bc_screen[1] + textures[2][1] * bc_screen[2];
//                 color.bgra[2] = textures[0][2] * bc_screen[0] + textures[1][2] * bc_screen[1] + textures[2][2] * bc_screen[2];
//                 color.bgra[3] = textures[0][3] * bc_screen[0] + textures[1][3] * bc_screen[1] + textures[2][3] * bc_screen[2];
//                 image.set(P.x, P.y, color * intensity);
//             }
//         }
//     }
// }

int main(int argc, char **argv)
{
    TGAImage image(width, height, TGAImage::RGB);
    float *zbuffer = new float[width * height];
    for (int i = 0; i < width * height; i++)
        zbuffer[i] = -std::numeric_limits<float>::max();
    if (argc > 1)
    {
        model = new Model(argv[1]);
        texture = new Texture(argv[2]);
    }
    else
    {
        model = new Model("obj\\african_head\\african_head.obj");
        texture = new Texture("obj\\african_head\\african_head_diffuse.tga");
    }

    ModelView = getView(cameraPos, cameraCenter, cameraUp);
    Projection = getProjection(-0.05, -50, 30, 1);
    Viewport = getViewport(width, height);

    GouraudShader shader;
    for (int i = 0; i < model->nfaces(); i++)
    {
        Vec4f pts[3];
        Vec3f normals[3];
        TGAColor textures[3];
        for (int j = 0; j < 3; j++)
        {
            pts[j] = shader.vertex(i, j);
            normals[j] = model->normal(i, j);
            textures[j] = texture->uv(model->texture(i, j));
        }
        triangle(pts, shader, image, zbuffer);
    }
    image.flip_vertically();
    image.write_tga_file("test.tga");
    delete model;
    delete texture;
    return 0;
}