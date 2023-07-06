#include <algorithm>
#include <ctime>
#include "tgaimage.h"
#include "geometry.h"
#include "model.h"

const double PI = acos(-1);

const int width = 800;
const int height = 800;
const TGAColor white(255, 255, 255, 255);
const TGAColor red(255, 0, 0, 255);
Model *model;
Vec3f light_dir(0, 0, -1);
float *zbuffer;

const Vec3f cameraPos(0, 0, 3);
const Vec3f cameraDir(0, 0, -1);
const Vec3f cameraUp(0, 1, 0);

Vec3f m2v(Matrix m)
{
    return Vec3f(m[0][0] / m[3][0], m[1][0] / m[3][0], m[2][0] / m[3][0]);
}
Matrix v2m(Vec3f v)
{
    Matrix mat = Matrix(4, 1);
    mat[0][0] = v[0];
    mat[1][0] = v[1];
    mat[2][0] = v[2];
    mat[3][0] = 1.f;
    return mat;
}
Matrix getModel(Vec3f scale)
{
    Matrix S = Matrix::identity(4);
    S[0][0] = scale[0];
    S[1][1] = scale[1];
    S[2][2] = scale[2];
    return S;
}
Matrix getView(Vec3f pos, Vec3f dir, Vec3f up)
{
    Matrix viewT = Matrix::identity(4);
    viewT[0][3] = -pos.x;
    viewT[1][3] = -pos.y;
    viewT[2][3] = -pos.z;
    Matrix viewR = Matrix::identity(4);
    Vec3f t = dir ^ up;
    viewR[0][0] = t.x;
    viewR[0][1] = t.y;
    viewR[0][2] = t.z;

    viewR[1][0] = up.x;
    viewR[1][1] = up.y;
    viewR[1][2] = up.z;

    viewR[2][0] = -dir.x;
    viewR[2][1] = -dir.y;
    viewR[2][2] = -dir.z;
    return viewR * viewT;
}
Matrix getProjection(float near, float far, float fov, float aspect)
{
    float angle = fov / 180.0 * PI;
    Matrix P2O = Matrix::identity(4);
    P2O[0][0] = near;
    P2O[1][1] = near;
    P2O[2][2] = near + far;
    P2O[2][3] = -near * far;
    P2O[3][2] = 1;
    Matrix O = Matrix::identity(4);
    float top = -near * tan(angle);
    float bottom = -top;
    float right = top / aspect;
    float left = -right;
    O[0][0] = 2.f / (right - left);
    O[1][1] = 2.f / (top - bottom);
    O[2][2] = 2.f / (far - near);
    Matrix T = Matrix::identity(4);
    T[0][3] = -(left + right) / 2.f;
    T[1][3] = -(top + bottom) / 2.f;
    T[2][3] = -(near + far) / 2.f;
    return O * T * P2O;
}
int getIndex(int x, int y)
{
    return width * y + x;
}
Matrix getViewport()
{
    Matrix vp = Matrix::identity(4);
    vp[0][0] = width / 2.f;
    vp[1][1] = height / 2.f;
    vp[0][3] = width / 2.f;
    vp[1][3] = height / 2.f;
    return vp;
}
Vec3f world2screen(Vec3f v)
{
    return Vec3f(int((v.x + 1.) * width / 2. + .5), int((v.y + 1.) * height / 2. + .5), v.z);
}
Vec3f barycentric(Vec3f A, Vec3f B, Vec3f C, Vec3f P)
{
    Vec3f AB = B - A, AC = C - A, PA = A - P;
    Vec3f u = Vec3f(AB.x, AC.x, PA.x), v = Vec3f(AB.y, AC.y, PA.y);
    Vec3f UV = u ^ v;
    if (std::fabs(UV.z) > 1e-2)
        return Vec3f((1.f - (UV.x + UV.y) / UV.z), UV.y / UV.z, UV.x / UV.z);
    return Vec3f(1, 1, -1);
}

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
            pts[i][j] = int(pts[i][j] + .5);
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

int main(int argc, char **argv)
{
    srand(time(0));
    TGAImage render(width, height, TGAImage::RGB);
    zbuffer = new float[width * height];
    for (int i = 0; i < width * height; i++)
        zbuffer[i] = -std::numeric_limits<float>::max();
    model = new Model("obj\\african_head\\african_head.obj");
    Matrix mod = getModel(Vec3f(1, 1, 1));
    Matrix view = getView(cameraPos, cameraDir, cameraUp);
    Matrix projection = getProjection(-0.05, -50, 30, 1);
    Matrix viewport = getViewport();
    for (int i = 0; i < model->nfaces(); i++)
    {
        std::vector<int> f = model->face(i);
        Vec3f pts[3], worldCoords[3];
        for (int j = 0; j < 3; j++)
        {
            worldCoords[j] = model->vert(f[j]);
            // pts[j] = world2screen((model->vert(f[j])));
            pts[j] = m2v(viewport * projection * view * mod * v2m(model->vert(f[j])));
        }

        // 计算法线
        Vec3f n = (worldCoords[2] - worldCoords[0]) ^ (worldCoords[1] - worldCoords[0]);
        n.normalize();
        float intensity = n * light_dir;
        if (intensity > 0)
            triangle(pts, zbuffer, render, TGAColor(255 * intensity, 255 * intensity, 255 * intensity, 255));
    }
    render.flip_vertically();
    render.write_tga_file("test.tga");
    delete model;
    return 0;
}