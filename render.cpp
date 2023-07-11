
#include "geometry.h"
#include "render.h"
#include "tgaimage.h"
#include <algorithm>

Matrix ModelView;
Matrix Viewport;
Matrix Projection;

IShader::~IShader()
{
}

void getView(Vec3f pos, Vec3f center, Vec3f up)
{
    Matrix viewT = Matrix::identity();
    viewT[0][3] = -pos.x;
    viewT[1][3] = -pos.y;
    viewT[2][3] = -pos.z;
    Matrix viewR = Matrix::identity();
    Vec3f dir = center - pos;
    dir.normalize();
    Vec3f t = cross(dir, up);
    up = cross(t, dir);
    viewR[0][0] = t.x;
    viewR[0][1] = t.y;
    viewR[0][2] = t.z;

    viewR[1][0] = up.x;
    viewR[1][1] = up.y;
    viewR[1][2] = up.z;

    viewR[2][0] = -dir.x;
    viewR[2][1] = -dir.y;
    viewR[2][2] = -dir.z;
    ModelView = viewR * viewT;
}

void getProjection(float near, float far, float fov, float aspect)
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
    Projection = O * T * P2O;
}
void getViewport(int width, int height)
{
    Matrix vp = Matrix::identity();
    vp[0][0] = width / 2.f;
    vp[1][1] = height / 2.f;
    vp[0][3] = width / 2.f;
    vp[1][3] = height / 2.f;
    vp[2][3] = 255.f / 2.f;
    vp[2][2] = 255.f / 2.f;
    Viewport = vp;
}

Render::Render(int width, int height, IShader *shader, MSAA msaa)
{
    this->shader = shader;
    this->width = width;
    this->height = height;
    this->msaa = msaa;
    image = new TGAImage(width, height, TGAImage::RGB);
    zbuffer = new float[width * height];
    for (int i = 0; i < width * height; i++)
        zbuffer[i] = -std::numeric_limits<float>::max();
    superImage = new TGAImage(width * msaa, height * msaa, TGAImage::RGB);
    superZbuffer = new float[width * height * msaa * msaa];
    for (int i = 0; i < width * height * msaa * msaa; i++)
        superZbuffer[i] = -std::numeric_limits<float>::max();
}

Render::~Render()
{
    delete image;
    delete superImage;
    delete[] zbuffer;
    delete[] superZbuffer;
}

static Vec3f barycentric(Vec2f A, Vec2f B, Vec2f C, Vec2f P)
{
    Vec2f AB = B - A, AC = C - A, PA = A - P;
    Vec3f u = Vec3f(AB.x, AC.x, PA.x), v = Vec3f(AB.y, AC.y, PA.y);
    Vec3f UV = cross(u, v);
    if (std::fabs(UV.z) > 1e-2)
        return Vec3f((1.f - (UV.x + UV.y) / UV.z), UV.x / UV.z, UV.y / UV.z);
    return Vec3f(1, 1, -1);
}
void Render::triangle(mat<4, 3, float> &clipc)
{
    mat<3, 4, float> pts = (Viewport * clipc).transpose(); // transposed to ease access to each of the points
    mat<3, 2, float> pts2;
    for (int i = 0; i < 3; i++)
        pts2[i] = proj<2>(pts[i] / pts[i][3]);

    Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    Vec2f clamp(image->get_width() - 1, image->get_height() - 1);
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            bboxmin[j] = std::max(0.f, std::min(bboxmin[j], pts2[i][j]));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts2[i][j]));
        }
    }
    Vec2i P;
    TGAColor color(0, 0, 0, 255), rColor(0, 0, 0, 255);
    for (P.x = int(bboxmin.x); P.x <= int(bboxmax.x + .5); P.x++)
    {
        for (P.y = int(bboxmin.y); P.y <= int(bboxmax.y + .5); P.y++)
        {
            for (int i = 0; i < msaa; i++)
            {
                for (int j = 0; j < msaa; j++)
                {
                    color = {0, 0, 0, 255};
                    Vec2i p;
                    p.x = P.x + 1.f / msaa * i + 1.f / msaa / 2.f, p.y = P.y + 1.f / msaa * j + 1.f / msaa / 2.f;
                    Vec3f bc_screen = barycentric(proj<2>(pts[0] / pts[0][3]), proj<2>(pts[1] / pts[1][3]), proj<2>(pts[2] / pts[2][3]), proj<2>(p));
                    Vec3f bc_clip = Vec3f(bc_screen.x / pts[0][3], bc_screen.y / pts[1][3], bc_screen.z / pts[2][3]);
                    bc_clip = bc_clip / (bc_clip.x + bc_clip.y + bc_clip.z);
                    if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
                        continue;
                    float frag_depth = 0;
                    for (int k = 0; k < 3; k++)
                        frag_depth += clipc[2][k] / clipc[3][k] * bc_clip[k];
                    int idx = getSuperIndex(int(P.x * msaa + i), int(P.y * msaa + j));
                    if (superZbuffer[idx] < frag_depth)
                    {

                        bool discard = shader->fragment(bc_clip, color);
                        if (!discard)
                        {
                            superZbuffer[idx] = frag_depth;
                            superImage->set(P.x * msaa + i, P.y * msaa + j, color);
                            rColor = {0, 0, 0, 255};
                            for (int ii = 0; ii < msaa; ii++)
                            {
                                for (int jj = 0; jj < msaa; jj++)
                                {
                                    rColor = rColor + superImage->get(P.x * msaa + ii, P.y * msaa + jj) * (1.f / msaa / msaa);
                                }
                            }
                            image->set(P.x, P.y, rColor);
                        }
                    }
                }
            }
        }
    }
}
