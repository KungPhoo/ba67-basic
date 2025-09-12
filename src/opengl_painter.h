#pragma once
#include <cstdint>
#include <cstddef>

class OpenGLPainter {
public:
    void initGL();
    void draw(size_t winWidth, size_t winHeight, size_t pixWidth, size_t pixHeight, uint32_t* pixelsBGRA, uint32_t borderColorBGRA);
    void shutdownGL();
};