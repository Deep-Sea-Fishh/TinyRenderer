#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"

class Model
{
private:
    std::vector<Vec3f> verts;              // 点集
    std::vector<Vec3f> normals;            // 法线集
    std::vector<std::vector<Vec3i>> faces; // 面集
    std::vector<Vec2f> textures;           // 材质
public:
    Model(const char *fileName);
    ~Model();
    int nverts();
    int nfaces();
    Vec3f vert(int idx);
    Vec3f normal(int idx);
    std::vector<Vec3i> face(int idx);
    Vec2f texture(int idx);
};

#endif