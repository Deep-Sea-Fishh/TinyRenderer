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

实现绘制三角形的功能。

##### 方法1：扫线

对一个三角形，我们可以细分为很多条线，依次画线即可将三角形填满。

我们可以将三个顶点按y坐标排序，从底向上画每一条线。

```cpp
void triangle(Vec2i *t, TGAImage &image, TGAColor color)
{
  std::sort(t, t + 2, [](Vec2i u, Vec2i v)
            { return u.y < v.y; });
  int totalHeight = t[2].y - t[0].y;
  for (int y = t[0].y; y < t[1].y; y++)
  {
      int segmentHeight = t[1].y - t[0].y;
      float alpha = (float)(y - t[0].y) / totalHeight;
      float beta = (float)(y - t[0].y) / segmentHeight;
      Vec2i A = t[0] + (t[2] - t[0]) * alpha;
      Vec2i B = t[0] + (t[1] - t[0]) * beta;
      line(A.x, y, B.x, y, image, color);
  }
}
```



##### 方法2：重心坐标

在重心坐标下，一个点P如果在三角形内部，则有P=uA+vB+wC，其中，$0\le u,v,w\le 1$且$u+v+w==1$。

先找出三角形的包围盒，遍历包围盒中的每一个点，判断该点是否在三角形内部，如果在，则绘制该像素点。

设$P=A+u\vec{AB}+v\vec{AC},即u\vec{AB}+v\vec{AC}+\vec{PA}==0$。

有下面两个式子：

$uAB_x+vAC_x+PA_x==0$

$uAB_y+vAC_y+PA_y==0$

写成矩阵的形式：
$$
\left[
\begin{matrix}
u & v & 1
\end{matrix}
\right]
·
\left[
\begin{matrix}
AB_x \\
AC_x \\
PA_x
\end{matrix}
\right]
=0
$$

$$
\left[
\begin{matrix}
u & v & 1
\end{matrix}
\right]
·
\left[
\begin{matrix}
AB_y \\
AC_y \\
PA_y
\end{matrix}
\right]
=0
$$

即找出一个与$[AB_x,AC_x,PA_x]$和$[AB_y,AC_y,PA_y]$同时垂直的向量，求叉积即可。

则最终有$P=(1-u-v)A+vB+uC$

```
Vec3f barycentric(int x, int y, Vec2i *t)
{
    Vec2i p(x, y);
    Vec2i AB = t[1] - t[0], AC = t[2] - t[0], PA = t[0] - p;
    Vec3f u(AB.x, AC.x, PA.x), v(AB.y, AC.y, PA.y);
    Vec3f uv = u ^ v;
    if (uv.z < 0)
        return {1, 1, -1};
    return Vec3f(1. - (uv.x + uv.y) / uv.z, uv.y / uv.z, uv.x / uv.z);
}
```



##### 计算光照

对于每个三角形，计算出法线并标准化，使用法线和光照的点乘作为光照强度。
