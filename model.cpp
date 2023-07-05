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
        else if (!line.compare(0, 2, "f ")) // 面
        {
            iss >> trash;
            int itrash, idx;
            std::vector<int> f;
            while (iss >> idx >> trash >> itrash >> trash >> itrash)
            {
                idx--;
                f.push_back(idx);
            }
            faces.push_back(f);
        }
    }
    std::cerr << "# v# " << verts.size() << " f# " << faces.size() << std::endl;
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

Vec3f Model::vert(int idx)
{
    return verts[idx];
}

std::vector<int> Model::face(int idx)
{
    return faces[idx];
}
