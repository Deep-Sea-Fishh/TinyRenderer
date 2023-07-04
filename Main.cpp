#include "tgaimage.h"

const TGAColor white(255, 255, 255, 255);
const TGAColor red(255, 0, 0, 255);
int main(int argc, char **argv)
{
    TGAImage image(800, 600, TGAImage::RGB);
    for (int i = 300; i < 400; i++)
        for (int j = 0; j < 800; j++)
            image.set(j, i, red);
    image.flip_vertically();
    image.write_tga_file("test.tga");
    return 0;
}