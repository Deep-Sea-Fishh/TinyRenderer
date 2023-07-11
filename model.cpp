#include "model.h"
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>

Model::Model(std::string fileName)
{
    std::ifstream in;
    in.open(fileName + ".obj", std::ifstream::in);
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
            uvs.push_back(v);
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
    // 读取diffuse
    diffuse = new Texture((fileName + "_diffuse.tga").c_str());
    // 读取specular
    specular = new Texture((fileName + "_spec.tga").c_str());
    // 读取nm
    nm = new Texture((fileName + "_nm.tga").c_str());
    // 读取nm_tangent
    nm_tangent = new Texture((fileName + "_nm_tangent.tga").c_str());
    std::cerr
        << "# v# " << verts.size() << "# vt# " << uvs.size() << " f# " << faces.size() << std::endl;
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

TGAColor Model::diff(int iface, int nthvert)
{
    return diffuse->uv(uv(iface, nthvert));
}

TGAColor Model::diff(Vec2f uv)
{
    return diffuse->uv(uv);
}

TGAColor Model::spec(int iface, int nthvert)
{
    return specular->uv(uv(iface, nthvert));
}

TGAColor Model::spec(Vec2f uv)
{
    return specular->uv(uv);
}

Vec3f Model::normal(int iface, int nthvert)
{
    return normals[faces[iface][nthvert].z];
}

Vec3f Model::normal(Vec2f uv)
{
    TGAColor tmp = nm->uv(uv);
    Vec3f res;
    for (int i = 0; i < 3; i++)
        res[i] = tmp[3 - i - 1] / 255.f * 2 - 1;
    return res;
}

Vec3f Model::normal_tangent(int iface, int nthvert)
{
    TGAColor tmp = nm_tangent->uv(uv(iface, nthvert));
    Vec3f res;
    for (int i = 0; i < 3; i++)
        res[i] = tmp[3 - i - 1] / 255.f * 2 - 1;
    return res;
}

Vec3f Model::normal_tangent(Vec2f uv)
{
    TGAColor tmp = nm_tangent->uv(uv);
    Vec3f res;
    for (int i = 0; i < 3; i++)
        res[i] = tmp[3 - i - 1] / 255.f * 2 - 1;
    return res;
}

std::vector<Vec3i> Model::face(int idx)
{
    return faces[idx];
}

Vec2f Model::uv(int iface, int nthvert)
{
    return uvs[faces[iface][nthvert].y];
}
