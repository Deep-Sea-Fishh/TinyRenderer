
#include "geometry.h"
#include "myGL.h"
#include "tgaimage.h"
#include <algorithm>

IShader::~IShader()
{
}

Matrix getModel(Vec3f scale)
{
    Matrix S = Matrix::identity();
    S[0][0] = scale[0];
    S[1][1] = scale[1];
    S[2][2] = scale[2];
    return S;
}
Matrix getView(Vec3f pos, Vec3f center, Vec3f up)
{
    Matrix viewT = Matrix::identity();
    viewT[0][3] = -pos.x;
    viewT[1][3] = -pos.y;
    viewT[2][3] = -pos.z;
    Matrix viewR = Matrix::identity();
    Vec3f dir = center - pos;
    dir.normalize();
    Vec3f t = cross(dir, up);
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
    Matrix P2O = Matrix::identity();
    P2O[0][0] = near;
    P2O[1][1] = near;
    P2O[2][2] = near + far;
    P2O[2][3] = -near * far;
    P2O[3][2] = 1;
    P2O[3][3] = 0;
    Matrix O = Matrix::identity();
    float top = -near * tan(angle);
    float bottom = -top;
    float right = top / aspect;
    float left = -right;
    O[0][0] = 2.f / (right - left);
    O[1][1] = 2.f / (top - bottom);
    O[2][2] = 2.f / (near - far);
    Matrix T = Matrix::identity();
    T[0][3] = -(left + right) / 2.f;
    T[1][3] = -(top + bottom) / 2.f;
    T[2][3] = -(near + far) / 2.f;
    return O * T * P2O;
}
Matrix getViewport(int width, int height)
{
    Matrix vp = Matrix::identity();
    vp[0][0] = width / 2.f;
    vp[1][1] = height / 2.f;
    vp[0][3] = width / 2.f;
    vp[1][3] = height / 2.f;
    vp[2][3] = 255.f / 2.f;
    vp[2][2] = 255.f / 2.f;
    return vp;
}

Vec3f barycentric(Vec2f A, Vec2f B, Vec2f C, Vec2f P)
{
    Vec2f AB = B - A, AC = C - A, PA = A - P;
    Vec3f u = Vec3f(AB.x, AC.x, PA.x), v = Vec3f(AB.y, AC.y, PA.y);
    Vec3f UV = cross(u, v);
    if (std::fabs(UV.z) > 1e-2)
        return Vec3f((1.f - (UV.x + UV.y) / UV.z), UV.x / UV.z, UV.y / UV.z);
    return Vec3f(1, 1, -1);
}

void triangle(Vec4f *pts, IShader &shader, TGAImage &image, float *zbuffer)
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

            bbomin[j] = std::max(0.f, std::min(bbomin[j], pts[i][j] / pts[i][3]));
            bbomax[j] = std::min(clamp[j], std::max(bbomax[j], pts[i][j] / pts[i][3]));
        }
    }
    Vec3f P;
    TGAColor color;
    for (P.x = int(bbomin.x); P.x <= int(bbomax.x + .5); P.x++)
    {
        for (P.y = int(bbomin.y); P.y <= int(bbomax.y + .5); P.y++)
        {
            Vec3f bc_screen = barycentric(proj<2>(pts[0] / pts[0][3]), proj<2>(pts[1] / pts[1][3]), proj<2>(pts[2] / pts[2][3]), proj<2>(P));
            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
                continue;
            P.z = 0;
            for (int i = 0; i < 3; i++)
                P.z += pts[i][2] / pts[i][3] * bc_screen[i];
            if (zbuffer[int(P.y * image.get_width() + P.x + .5)] < P.z)
            {

                bool discard = shader.fragment(bc_screen, color);
                if (!discard)
                {
                    zbuffer[int(P.y * image.get_width() + P.x + .5)] = P.z;
                    image.set(P.x, P.y, color);
                }
            }
            // if (zbuffer.get(P.x, P.y)[0] < P.z)
            // {

            //     bool discard = shader.fragment(bc_screen, color);
            //     if (!discard)
            //     {
            //         zbuffer.get(P.x, P.y)[0] = P.z;
            //         image.set(P.x, P.y, color);
            //     }
            // }
        }
    }
}