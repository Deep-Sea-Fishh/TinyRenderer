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

则最终有$P=(1-u-v)A+uB+vC$

```
Vec3f barycentric(int x, int y, Vec2i *t)
{
    Vec2i p(x, y);
    Vec2i AB = t[1] - t[0], AC = t[2] - t[0], PA = t[0] - p;
    Vec3f u(AB.x, AC.x, PA.x), v(AB.y, AC.y, PA.y);
    Vec3f uv = u ^ v;
    if (uv.z < 0)
        return {1, 1, -1};
    return Vec3f(1. - (uv.x + uv.y) / uv.z, uv.x / uv.z, uv.y / uv.z);
}
```



##### 计算光照

对于每个三角形，计算出法线并标准化，使用法线和光照的点乘作为光照强度。

### Z缓冲

为了解决点和点之间前后关系不正确的问题，需要引入Z缓冲，即在渲染时，对于每一个像素点都存一下该像素点当前最靠前的深度是多少，在画三角形内的点时，和z缓冲区中该点的深度值进行比较，如果当前要绘制的点更靠前，则覆盖z缓冲区的原值，并将该点的颜色绘制在framebuffer上。

## Transformation 变换

### 缩放

各个坐标乘以各自的缩放系数，矩阵形式如下：
$$
\begin{bmatrix}
x'\\
y'\\
\end{bmatrix}
=
\begin{bmatrix}
s_x&0\\
0&s_y\\
\end{bmatrix}
*
\begin{bmatrix}
x\\
y\\
\end{bmatrix}
$$

### 切变

![image-20230203113306294](C:\Users\17633\AppData\Roaming\Typora\typora-user-images\image-20230203113306294.png)
$$
\begin{bmatrix}
x'\\
y'\\
\end{bmatrix}
=
\begin{bmatrix}
1&a\\
0&1\\
\end{bmatrix}
*
\begin{bmatrix}
x\\
y\\

\end{bmatrix}
$$

### 旋转

默认按原点逆时针旋转。

如果需要按任意点进行旋转，我们可以先将旋转中心平移到原点，旋转后在平移回去。
$$
\begin{bmatrix}
x'\\
y'\\
\end{bmatrix}
=
\begin{bmatrix}
cosθ&-sinθ\\
sinθ&cosθ\\
\end{bmatrix}
*
\begin{bmatrix}
x\\
y\\

\end{bmatrix}
$$
考虑旋转$-θ$
$$
\begin{bmatrix}
x'\\
y'\\
\end{bmatrix}
=
\begin{bmatrix}
cosθ&sinθ\\
-sinθ&cosθ\\
\end{bmatrix}
*
\begin{bmatrix}
x\\
y\\

\end{bmatrix}
$$
可以发现：

$R_{-θ}={R_{θ}}^T$，同时，由于旋转-θ和旋转θ是逆变换，所以有$R_{-θ}={R_θ}^{-1}$，也就有${R_{θ}}^T={R_θ}^{-1}$，正交矩阵。

对于三维坐标：

绕$x$轴旋转
$$
\begin{bmatrix}
x'\\
y'\\
z'\\
\end{bmatrix}
=
\begin{bmatrix}
1&0&0\\
0&cosθ&-sinθ\\
0&sinθ&cosθ\\
\end{bmatrix}
*
\begin{bmatrix}
x\\
y\\
z\\
\end{bmatrix}
$$
绕$y$轴旋转
$$
\begin{bmatrix}
x'\\
y'\\
z'\\
\end{bmatrix}
=
\begin{bmatrix}
cosθ&0&sinθ\\
0&1&0\\
-sinθ&0&cosθ\\
\end{bmatrix}
*
\begin{bmatrix}
x\\
y\\
z\\
\end{bmatrix}
$$


绕$z$轴旋转
$$
\begin{bmatrix}
x'\\
y'\\
z'\\
\end{bmatrix}
=
\begin{bmatrix}
cosθ&-sinθ&0\\
sinθ&cosθ&0\\
0&0&1\\
\end{bmatrix}
*
\begin{bmatrix}
x\\
y\\
z\\
\end{bmatrix}
$$
绕任意轴旋转—罗德里格斯旋转公式
$$
R_{(\pmb{n},θ)}=cos(θ)I+(1-cos(θ))\pmb{n}\pmb{n^T}+sin(θ)
\begin{pmatrix}
0&\pmb{-n_z}&\pmb{n_y}\\
\pmb{n_z}&0&\pmb{-n_x}\\
\pmb{-n_y}&\pmb{n_x}&0\\
\end{pmatrix}
$$

### 平移变换

![image-20230203114450906](C:\Users\17633\AppData\Roaming\Typora\typora-user-images\image-20230203114450906.png)
$$
\begin{bmatrix}
x'\\
y'\\
z'\\
\end{bmatrix}
=
\begin{bmatrix}
a&b&c\\
d&e&f\\
g&h&i\\
\end{bmatrix}
*
\begin{bmatrix}
x\\
y\\
z\\
\end{bmatrix}
+
\begin{bmatrix}
t_x\\
t_y\\
t_z\\
\end{bmatrix}
$$

### 齐次坐标

通过观察上述变换，我们可以发现缩放、切变、旋转变换后的新坐标都可以表示成旧坐标左乘一个变换矩阵，而平移变换不可以，我们希望这些变换都可以用一种形式进行表示，所以这里引入了齐次坐标。

我们将坐标增加一维$w$，对于点来说，$w$坐标为1，对于向量来说，$w$坐标为0。同时，变换矩阵也增加一个维度。

若点的$w$坐标不为1，我们需要将其标准化，即将每个维度的坐标都除以$w$。
$$
\begin{bmatrix}
x'\\
y'\\
z'\\
1\\
\end{bmatrix}
=
\begin{bmatrix}
a&b&c&t_x\\
d&e&f&t_y\\
g&h&i&t_z\\
0&0&0&1\\
\end{bmatrix}
*
\begin{bmatrix}
x\\
y\\
z\\
1\\
\end{bmatrix}
$$
其中，$a、b、c、d、e、f、g、h、i$控制着之前的线性变换，$t_x、t_y、t_z$控制平移变换。

这些变换统称为仿射变换。

### 逆变换

左乘变换矩阵的逆矩阵。

### 变换组合

由于矩阵乘法不满足交换律，所以矩阵的顺序很重要，当有多个变换矩阵时，点的坐标会从右到左依次应用变换矩阵。

同时，我们知道矩阵满足结合律，我们可以先将所有变换矩阵进行乘法运算，在之后用坐标左乘结合后的矩阵，而不是一个矩阵一个矩阵地左乘，以此来加速运算。

### MVP

#### Model Transformation  放置模型

#### View Transformation  放置相机

我们可以将相机的移动转化为固定相机移动其他物体。这里我们将相机固定在原点，面向-z方向，y轴向上。

首先我们需要将相机移至原点，即左乘如下矩阵：
$$
T_{view}=
\begin{bmatrix}
1&0&0&-x_e\\
0&1&0&-y_e\\
0&0&1&-z_e\\
0&0&0&1\\
\end{bmatrix}
$$
接着将$g$旋转至$-z$，$t$旋转至$y$，$g\times t$旋转至$x$。考虑到正着不好做，我们可以考虑逆操作，即将$z$旋转至$-g$，$y$旋转至$t$，$x$旋转至$g\times t$。

则旋转矩阵的逆矩阵如下
$$
{R_{view}}^{-1}=
\begin{bmatrix}
x_{g\times t}&x_t&x_{-g}&0\\
y_{g\times t}&y_t&y_{-g}&0\\
z_{g\times t}&z_t&z_{-g}&0\\
0&0&0&1\\
\end{bmatrix}
$$
由于旋转矩阵是正交矩阵，则
$$
R_{view}=
{{R_{view}}^{-1}}^T=

\begin{bmatrix}
x_{g\times t}&y_{g\times t}&z_{g\times t}&0\\
x_t&y_t&z_t&0\\
x_{-g}&y_{-g}&z_{-g}&0\\
0&0&0&1\\
\end{bmatrix}
$$
所以相机变换相当于左乘$R_{view}T_{view}$。

#### Projection Transformation  投影变换

##### 正交投影

![image-20230204101011520](C:\Users\17633\AppData\Roaming\Typora\typora-user-images\image-20230204101011520.png)

将矩形中心平移至原点，并缩放为边长为2的正方体。
$$
M_{ortho}=
\begin{bmatrix}
\frac{2}{r-l}&0&0&0\\
0&\frac{2}{t-b}&0&0\\
0&0&\frac{2}{n-f}&0\\
0&0&0&1\\
\end{bmatrix}
\begin{bmatrix}
1&0&0&-\frac{l+r}{2}\\
0&1&0&-\frac{t+b}{2}\\
0&0&1&-\frac{n+f}{2}\\
0&0&0&1\\
\end{bmatrix}
$$

##### 透视投影

![image-20230204102912786](C:\Users\17633\AppData\Roaming\Typora\typora-user-images\image-20230204102912786.png)

首先将左侧的Frustum压成右侧的矩形，在使用正交投影即可。下面介绍如何将Frustum压成矩形。

![image-20230204102928968](C:\Users\17633\AppData\Roaming\Typora\typora-user-images\image-20230204102928968.png)

![image-20230204102947983](C:\Users\17633\AppData\Roaming\Typora\typora-user-images\image-20230204102947983.png)

我们要求一个矩阵，使得点的坐标左乘该矩阵后能够得到右边的结果。

![image-20230204103043916](C:\Users\17633\AppData\Roaming\Typora\typora-user-images\image-20230204103043916.png)

我们由$x,y,w$坐标可以得到变换矩阵的$1,2,4$行，第三行如何得到呢？

我们考虑$z$坐标，对于近平面和远平面的中点，他们变换后的结果和$x,y$没有关系，所以第三行的前两个系数应该是0，设后两个参数分别为$A,B$,并且变换后他们的$z$坐标是不变的，所以有如下方程组：
$$
An+B=n^2\\
Af+B=f^2\\
$$
解得
$$
A=n+f\\
B=-nf\\
$$
所以我们现在得到了如下变换矩阵：
$$
M_{persp->ortho}=
\begin{bmatrix}
n&0&0&0\\
0&n&0&0\\
0&0&n+f&-nf\\
0&0&1&0\\
\end{bmatrix}
$$

## 

## Rasterizer 光栅化

### 视口变换

将$[-1,1]^3$的立方体映射到二维屏幕上。
$$
M_{viewport}=
\begin{bmatrix}
\frac{width}{2}&0&0&\frac{width}{2}\\
0&\frac{height}{2}&0&\frac{height}{2}\\
0&0&-1&0\\
0&0&0&1\\
\end{bmatrix}
$$
