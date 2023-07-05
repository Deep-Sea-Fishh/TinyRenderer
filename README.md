# TinyRenderer

### 模型

##### obj文件格式

一个OBJ文件可以包含顶点数据，自由形式的曲线/曲面属性，元素，自由形式的曲线/曲面体语句，自由形式曲面之间的连接，分组和显示/渲染属性信息。最常见的元素是**几何顶点、纹理坐标、顶点法线和多边形面**。

几何顶点的格式如下：

```
v x y z r g b
```

v是标识符，表示这一行为几何顶点数据，后面紧跟着三个坐标表示x,y,z坐标，后面可以再跟颜色。

纹理坐标

顶点法线

多边形面的格式如下：

```
f 顶点1索引/纹理1索引/法线1索引 顶点2索引/纹理2索引/法线2索引 顶点3索引/纹理3索引/法线3索引
```

##### Model类

model.h

```cpp
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
```

model.cpp

```cpp
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
                iss >> v.raw[i];
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

```



### 画线

实现绘制平面两点之间的线段

##### 方法1 

从起点开始，每次向终点移动一定的步长step，但是这对于步长有一定的要求，当步长过短，循环次数会变大，当步长过长，则绘制的线段会有间隙。

##### 方法2

假设$4x0,x1$的距离比$y0,y1$的距离更长，则我们将$x$从$x0$向$x1$循环，每次计算出当前y的位置，在这种情况下，循环次数$=abs(x0-x1)$，且绘制的线段不会有间隙。

##### 最终代码(优化后)

```cpp
void line(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color)
{
    bool steep = false;
    if (std::abs(x1 - x0) < std::abs(y1 - y0))
    {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    if (x0 > x1)
    {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    int dx = x1 - x0;
    int dy = y1 - y0;
    int y = y0;
    int error = 0;
    // float derror = std::abs(dy / float(dx));
    int derror = std::abs(dy) * 2;
    for (int x = x0; x <= x1; x++)
    {
        steep ? image.set(y, x, color) : image.set(x, y, color);
        error += derror;
        if (error > dx)
        {
            y += y1 > y0 ? 1 : -1;
            error -= 2 * dx;
        }
    }
}
```

使用画线函数绘制模型

  ![image-20230705091411436](C:\Users\17633\AppData\Roaming\Typora\typora-user-images\image-20230705091411436.png)

### 画三角形

