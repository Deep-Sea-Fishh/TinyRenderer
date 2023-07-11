#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include <string>
#include "texture.h"
#include "geometry.h"

class Model
{
private:
    std::vector<Vec3f> verts;              // 点集
    std::vector<Vec3f> normals;            // 法线集
    std::vector<std::vector<Vec3i>> faces; // 面集
    std::vector<Vec2f> uvs;                // 材质
    Texture *diffuse;
    Texture *specular;
    Texture *nm;
    Texture *nm_tangent;

public:
    Model(std::string fileName);
    ~Model();
    int nverts();
    int nfaces();
    Vec3f vert(int iface, int nthvert);
    TGAColor diff(int iface, int nthvert);
    TGAColor diff(Vec2f uv);
    TGAColor spec(int iface, int nthvert);
    TGAColor spec(Vec2f uv);
    Vec3f normal(int iface, int nthvert);
    Vec3f normal(Vec2f uv);
    Vec3f normal_tangent(int iface, int nthvert);
    Vec3f normal_tangent(Vec2f uv);
    std::vector<Vec3i> face(int idx);
    Vec2f uv(int iface, int nthvert);
};

#endif