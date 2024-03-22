#include "GL_types.h"

namespace OpenGL {
    
    void RenderTarget::Configure(int width, int height) {
        if (fbo != 0) {
            glDeleteFramebuffers(1, &fbo);
        }
        glGenFramebuffers(1, &fbo);
        this->width = width;
        this->height = height;
    }
}