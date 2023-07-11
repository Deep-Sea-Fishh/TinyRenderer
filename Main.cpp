#include <algorithm>
#include <ctime>
#include <string>
#include "tgaimage.h"
#include "geometry.h"
#include "model.h"
#include "texture.h"
#include "render.h"

const int width = 800;
const int height = 800;

Model *model = nullptr;
Vec3f light_dir(1, 1, 1);

const Vec3f cameraPos(1, 1, 3);
const Vec3f cameraCenter(0, 0, 0);
const Vec3f cameraUp(0, 1, 0);

int cnt = 0;
struct Shader : public IShader
{
    mat<2, 3, float> varying_uv;  // triangle uv coordinates, written by the vertex shader, read by the fragment shader
    mat<4, 3, float> varying_tri; // triangle coordinates (clip coordinates), written by VS, read by FS
    mat<3, 3, float> varying_nrm; // normal per vertex to be interpolated by FS
    mat<3, 3, float> ndc_tri;     // triangle in normalized device coordinates

    virtual Vec4f vertex(int iface, int nthvert)
    {
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        varying_nrm.set_col(nthvert, proj<3>((Projection * ModelView).invert_transpose() * embed<4>(model->normal(iface, nthvert), 0.f)));
        Vec4f gl_Vertex = Projection * ModelView * embed<4>(model->vert(iface, nthvert));
        varying_tri.set_col(nthvert, gl_Vertex);
        ndc_tri.set_col(nthvert, proj<3>(gl_Vertex / gl_Vertex[3]));
        return gl_Vertex;
    }

    virtual bool fragment(Vec3f bar, TGAColor &color)
    {

        float ka = 0.1, kd = 0.9, ks = 0.3, p = 60;
        TGAColor amb = {128, 128, 128, 255};

        Vec3f bn = (varying_nrm * bar).normalize();
        Vec2f uv = varying_uv * bar;
        Vec3f vtx = ndc_tri * bar;
        mat<3, 3, float> A;
        A[0] = ndc_tri.col(1) - ndc_tri.col(0);
        A[1] = ndc_tri.col(2) - ndc_tri.col(0);
        A[2] = bn;

        mat<3, 3, float> AI = A.invert();

        Vec3f i = AI * Vec3f(varying_uv[0][1] - varying_uv[0][0], varying_uv[0][2] - varying_uv[0][0], 0);
        Vec3f j = AI * Vec3f(varying_uv[1][1] - varying_uv[1][0], varying_uv[1][2] - varying_uv[1][0], 0);

        mat<3, 3, float> B;
        B.set_col(0, i.normalize());
        B.set_col(1, j.normalize());
        B.set_col(2, bn);
        Vec3f n = (B * model->normal(uv)).normalize();

        float diff = std::max(0.f, n * light_dir);

        // 计算高光
        Vec3f eyeDir = -vtx;
        Vec3f half = (light_dir + eyeDir).normalize();
        float spec = std::max(0.f, half * bn);

        color = amb * ka + model->diff(uv) * diff * kd + model->spec(uv) * std::pow(spec, p) * ks;

        return false;
    }
};
struct PhongShader : public IShader
{
    mat<3, 3, float> varying_nrm; // normal per vertex to be interpolated by FS
    mat<2, 3, float> varying_uv;  // triangle uv coordinates, written by the vertex shader, read by the fragment shader
    mat<3, 3, float> ndc_tri;     // triangle in normalized device coordinates
    mat<4, 3, float> varying_tri;
    virtual Vec4f vertex(int iface, int nthvert)
    {
        Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from .obj file
        gl_Vertex = Projection * ModelView * gl_Vertex;          // transform it to screen coordinates
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        varying_nrm.set_col(nthvert, proj<3>((Projection * ModelView).invert_transpose() * embed<4>(model->normal(iface, nthvert), 0.f)));
        ndc_tri.set_col(nthvert, proj<3>(gl_Vertex / gl_Vertex[3]));
        varying_tri.set_col(nthvert, gl_Vertex);
        return gl_Vertex;
    }

    virtual bool fragment(Vec3f bar, TGAColor &color)
    {

        Vec3f normal = varying_nrm * bar;
        Vec2f uv = varying_uv * bar;

        mat<3, 3, float> A;
        A[0] = ndc_tri.col(1) - ndc_tri.col(0);
        A[1] = ndc_tri.col(2) - ndc_tri.col(0);
        A[2] = normal;

        mat<3, 3, float> AI = A.invert();

        Vec3f i = AI * Vec3f(varying_uv[0][1] - varying_uv[0][0], varying_uv[0][2] - varying_uv[0][0], 0);
        Vec3f j = AI * Vec3f(varying_uv[1][1] - varying_uv[1][0], varying_uv[1][2] - varying_uv[1][0], 0);

        mat<3, 3, float> B;
        B.set_col(0, i.normalize());
        B.set_col(1, j.normalize());
        B.set_col(2, normal);

        Vec3f n = (B * model->normal(uv)).normalize();
        float intensity = n * light_dir;

        color = model->diff(uv) * intensity;
        return false; // no, we do not discard this pixel
    }
};
int main(int argc, char **argv)
{
    std::vector<Model *> models;
    if (argc > 1)
    {
        std::string file;
        for (int i = 1; i < argc; i++)
        {
            file = argv[i];
            model = new Model(("../" + file).c_str());
            models.push_back(model);
        }
    }
    else
    {
        model = new Model("../obj/african_head/african_head");
        models.push_back(model);
    }

    getView(cameraPos, cameraCenter, cameraUp);
    getProjection(-2, -20, 20, 1);
    getViewport(width, height);
    light_dir = proj<3>((Projection * ModelView).invert_transpose() * embed<4>(light_dir, 0.f)).normalize();
    PhongShader shader;
    Render *render = new Render(width, height, &shader, MSAA::TWO_TWO);
    for (int t = 0; t < std::max(1, argc - 1); t++)
    {
        model = models[t];
        for (int i = 0; i < model->nfaces(); i++)
        {
            for (int j = 0; j < 3; j++)
            {
                shader.vertex(i, j);
            }
            render->triangle(shader.varying_tri);
        }
    }
    std::cout << cnt << std::endl;
    render->getImage()->flip_vertically();
    render->getImage()->write_tga_file("TBN.tga");
    while (models.size())
    {
        delete models.back();
        models.pop_back();
    }
    return 0;
}