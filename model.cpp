#include "model.h"
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>

Model::Model(const char *fileName)
{
    std::ifstream in;
    in.open(fileName, std::ifstream::in);
    if (in.fail())
    {
        std::cerr << "打开文件失败,文件路径:" << fileName << std::endl;
        return;
    }
    std::string line;
    while (!in.eof())
    {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        char trash;
        if (!line.compare(0, 2, "v ")) // 点
        {
            iss >> trash;
            Vec3f v;
            for (int i = 0; i < 3; i++)
                iss >> v[i];
            verts.push_back(v);
        }
        else if (!line.compare(0, 3, "vt ")) // 点
        {
            iss >> trash >> trash;
            Vec2f v;
            for (int i = 0; i < 2; i++)
                iss >> v[i];
            while (iss >> trash)
                ;
            textures.push_back(v);
        }
        else if (!line.compare(0, 3, "vn ")) // 点
        {
            iss >> trash >> trash;
            Vec3f v;
            for (int i = 0; i < 3; i++)
                iss >> v[i];
            normals.push_back(v);
        }
        else if (!line.compare(0, 2, "f ")) // 面
        {
            iss >> trash;
            int itrash;
            std::vector<Vec3i> f;
            Vec3i idx;
            while (iss >> idx.x >> trash >> idx.y >> trash >> idx.z)
            {
                idx.x--;
                idx.y--;
                idx.z--;
                f.push_back(idx);
            }
            faces.push_back(f);
        }
    }
    std::cerr << "# v# " << verts.size() << "# vt# " << textures.size() << "# vn# " << normals.size() << " f# " << faces.size() << std::endl;
}

Model::~Model()
{
}

int Model::nverts()
{
    return verts.size();
}

int Model::nfaces()
{
    return faces.size();
}

Vec3f Model::vert(int iface, int nthvert)
{
    return verts[faces[iface][nthvert].x];
}

Vec3f Model::normal(int iface, int nthvert)
{
    return normals[faces[iface][nthvert].z];
}

std::vector<Vec3i> Model::face(int idx)
{
    return faces[idx];
}

Vec2f Model::texture(int iface, int nthvert)
{
    return textures[faces[iface][nthvert].y];
}
