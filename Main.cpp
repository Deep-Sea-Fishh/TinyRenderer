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

struct GouraudShader : public IShader
{
    Vec3f varying_intensity; // written by vertex shader, read by fragment shader
    TGAColor varying_color[3];
    virtual Vec4f vertex(int iface, int nthvert)
    {
        Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert));                                  // read the vertex from .obj file
        gl_Vertex = Viewport * Projection * ModelView * gl_Vertex;                                // transform it to screen coordinates
        varying_intensity[nthvert] = std::max(0.f, -(model->normal(iface, nthvert) * light_dir)); // get diffuse lighting intensity
        varying_color[nthvert] = texture->uv(model->texture(iface, nthvert));
        return gl_Vertex;
    }

    virtual bool fragment(Vec3f bar, TGAColor &color)
    {
        float intensity = varying_intensity * bar; // interpolate intensity for the current pixel
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 4; j++)
                color[j] += varying_color[i][j] * bar[i];
        color = color * intensity; // well duh
        return false;              // no, we do not discard this pixel
    }
};

struct PhoneShader : public IShader
{
    Vec3f varying_normal[3];
    Vec2f varying_texture[3];
    virtual Vec4f vertex(int iface, int nthvert)
    {
        Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert));   // read the vertex from .obj file
        gl_Vertex = Viewport * Projection * ModelView * gl_Vertex; // transform it to screen coordinates
        varying_normal[nthvert] = model->normal(iface, nthvert);
        varying_texture[nthvert] = model->texture(iface, nthvert);
        return gl_Vertex;
    }

    virtual bool fragment(Vec3f bar, TGAColor &color)
    {
        Vec3f normal;
        Vec2f tex;
        for (int i = 0; i < 3; i++)
        {
            normal = normal + varying_normal[i] * bar[i];
            tex = tex + varying_texture[i] * bar[i];
        }
        float intensity = -(normal * light_dir);
        color = texture->uv(tex) * intensity; // well duh
        return false;                         // no, we do not discard this pixel
    }
};

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

    PhoneShader shader;
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