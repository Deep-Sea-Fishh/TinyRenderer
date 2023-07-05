#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"

class Model
{
private:
    std::vector<Vec3f> verts;            // 点集
    std::vector<std::vector<int>> faces; // 面集
public:
    Model(const char *fileName);
    ~Model();
    int nverts();
    int nfaces();
    Vec3f vert(int idx);
    std::vector<int> face(int idx);
};

#endif