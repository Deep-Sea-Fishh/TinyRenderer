#include <algorithm>
#include <ctime>
#include <string>
#include "tgaimage.h"
#include "geometry.h"
#include "model.h"
#include "texture.h"
#include "myGL.h"

const int width = 800;
const int height = 800;

Model *model;
Texture *diffuse;
Texture *spec;
Vec3f light_dir(0, 0, -1);

Matrix ModelView;
Matrix Viewport;
Matrix Projection;

const Vec3f cameraPos(0, 0, 3);
const Vec3f cameraCenter(0, 0, 0);
const Vec3f cameraUp(0, 1, 0);

struct GouraudShader : public IShader
{
    TGAColor varying_color[3];
    virtual Vec4f vertex(int iface, int nthvert)
    {
        // 定义环境光,漫反射系数,镜面反射系数
        float ka = 0.1, kd = 0.6, ks = 0.5, p = 60;
        TGAColor amb = {128, 128, 128, 255};

        Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert));                       // read the vertex from .obj file
        gl_Vertex = Viewport * Projection * ModelView * gl_Vertex;                     // transform it to screen coordinates
        float intensity = std::max(0.f, -(model->normal(iface, nthvert) * light_dir)); // get diffuse lighting intensity
        // 计算半程向量
        Vec3f viewDir = (model->vert(iface, nthvert) - cameraPos);
        viewDir.normalize();
        Vec3f h = (light_dir + viewDir);
        h.normalize();
        // 计算n·h
        float ksIntensity = std::max(0.f, -(model->normal(iface, nthvert) * h));
        varying_color[nthvert] = amb * ka + diffuse->uv(model->texture(iface, nthvert)) * intensity * kd + spec->uv(model->texture(iface, nthvert)) * std::pow(ksIntensity, p) * ks;
        return gl_Vertex;
    }

    virtual bool fragment(Vec3f bar, TGAColor &color)
    {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 4; j++)
                color[j] += varying_color[i][j] * bar[i];
        return false; // no, we do not discard this pixel
    }
};

struct PhoneShader : public IShader
{
    Vec3f varying_normal[3];
    Vec2f varying_diffuse[3];
    Vec3f varying_vertex[3];
    virtual Vec4f vertex(int iface, int nthvert)
    {
        Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert));   // read the vertex from .obj file
        gl_Vertex = Viewport * Projection * ModelView * gl_Vertex; // transform it to screen coordinates
        varying_normal[nthvert] = model->normal(iface, nthvert);
        varying_diffuse[nthvert] = model->texture(iface, nthvert);
        varying_vertex[nthvert] = model->vert(iface, nthvert);
        return gl_Vertex;
    }

    virtual bool fragment(Vec3f bar, TGAColor &color)
    {
        // 定义环境光,漫反射系数,镜面反射系数
        float ka = 0.1, kd = 0.9, ks = 0.3, p = 60;
        TGAColor amb = {128, 128, 128, 255};

        Vec3f vtx;
        Vec3f normal = {0, 0, 0};
        Vec2f tex = {0, 0};
        for (int i = 0; i < 3; i++)
        {
            vtx = vtx + varying_vertex[i] * bar[i];
            normal = normal + varying_normal[i] * bar[i];
            tex = tex + varying_diffuse[i] * bar[i];
        }
        float intensity = -(normal * light_dir);

        // 计算半程向量
        Vec3f viewDir = vtx - cameraPos;
        viewDir.normalize();
        Vec3f h = (light_dir + viewDir);
        h.normalize();
        // 计算n·h
        float ksIntensity = std::max(0.f, -(normal * h));

        color = amb * ka + diffuse->uv(tex) * intensity * kd + spec->uv(tex) * std::pow(ksIntensity, p) * ks;
        return false; // no, we do not discard this pixel
    }
};

int main(int argc, char **argv)
{
    TGAImage image(width, height, TGAImage::RGB);
    float *zbuffer = new float[width * height];
    for (int i = 0; i < width * height; i++)
        zbuffer[i] = -std::numeric_limits<float>::max();

    std::vector<Model *> models;
    std::vector<Texture *> diffuses;
    std::vector<Texture *> specs;
    if (argc > 1)
    {
        std::string file;
        for (int i = 1; i < argc; i++)
        {
            file = argv[i];
            model = new Model((file + ".obj").c_str());
            diffuse = new Texture((file + "_diffuse.tga").c_str());
            spec = new Texture((file + "_spec.tga").c_str());
            models.push_back(model);
            diffuses.push_back(diffuse);
            specs.push_back(spec);
        }
    }
    else
    {
        model = new Model("obj\\african_head\\african_head.obj");
        diffuse = new Texture("obj\\african_head\\african_head_diffuse.tga");
        spec = new Texture("obj\\african_head\\african_head_spec.tga");
        models.push_back(model);
        diffuses.push_back(diffuse);
        specs.push_back(spec);
    }

    ModelView = getView(cameraPos, cameraCenter, cameraUp);
    Projection = getProjection(-1, -35, 30, 1);
    Viewport = getViewport(width, height);

    PhoneShader shader;
    for (int t = 0; t < std::max(1, argc - 1); t++)
    {
        model = models[t];
        diffuse = diffuses[t];
        spec = specs[t];
        for (int i = 0; i < model->nfaces(); i++)
        {
            Vec4f pts[3];
            Vec3f normals[3];
            TGAColor difs[3];
            for (int j = 0; j < 3; j++)
            {
                pts[j] = shader.vertex(i, j);
                normals[j] = model->normal(i, j);
                difs[j] = diffuse->uv(model->texture(i, j));
            }
            triangle(pts, shader, image, zbuffer);
        }
    }

    image.flip_vertically();
    image.write_tga_file("test.tga");
    while (models.size())
    {
        delete models.back();
        models.pop_back();
    }
    while (diffuses.size())
    {
        delete diffuses.back();
        diffuses.pop_back();
    }
    while (specs.size())
    {
        delete specs.back();
        specs.pop_back();
    }
    return 0;
}