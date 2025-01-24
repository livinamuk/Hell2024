#include "GL_CubeMap2.h"
#include <glad/glad.h>
#include "Defines.h"


void CubeMap2::Init(unsigned int size) {

    m_size = size;

    glGenFramebuffers(1, &m_ID);
    glGenTextures(1, &m_depthTex);
    glGenTextures(1, &m_colorTex);

    glBindTexture(GL_TEXTURE_CUBE_MAP, m_depthTex);
    glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_DEPTH_COMPONENT32F, size, size);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, m_ID);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depthTex, 0);

    glBindTexture(GL_TEXTURE_CUBE_MAP, m_colorTex);
    glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_RGBA32F, size, size);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_colorTex, 0);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_colorTex);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    //CheckStatus();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenTextures(1, &m_textureView);  // Generate a new texture for the view


    glTextureView(
        m_textureView,                  // The texture view handle
        GL_TEXTURE_2D_ARRAY,            // Alias it as a 2D texture array
        m_colorTex,                     // The original cubemap array texture
        GL_RGBA32F,                         // Format matches the original
        0,                              // Mip level start
        1,                              // Number of mipmap levels
        0,                              // First layer (start from the first layer)
        6                               // Number of layers (6 faces per cubemap)
    );
}

void CubeMap2::CheckStatus() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_ID);
    auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus == 36053)
        std::cout << "CUBEMAP FRAMEBUFFER: complete\n";
    if (fboStatus == 36054)
        std::cout << "CUBEMAP FRAMEBUFFER: incomplete attachment\n";
    if (fboStatus == 36057)
        std::cout << "CUBEMAP FRAMEBUFFER: incomplete dimensions\n";
    if (fboStatus == 36055)
        std::cout << "CUBEMAP FRAMEBUFFER: missing attachment\n";
    if (fboStatus == 36061)
        std::cout << "CUBEMAP FRAMEBUFFER: unsupported\n";
    auto glstatus = glGetError();
    if (glstatus != GL_NO_ERROR) {
        std::cout << "Error in GL call: " << glstatus << std::endl;
    }
}

void CubeMap2::Clear() {
    glViewport(0, 0, m_size, m_size);
    glBindFramebuffer(GL_FRAMEBUFFER, m_ID);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
