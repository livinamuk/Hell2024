#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace OpenGL {

    struct RenderTarget {

        GLuint fbo = { 0 };
        GLuint texture = { 0 };
        GLuint width = { 0 };
        GLuint height = { 0 };

        void Configure(int width, int height);
    };
}