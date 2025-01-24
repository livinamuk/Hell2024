#pragma once
#include "../Common/Types.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

struct FlipbookTexture {

    void Load(const std::string& path);

    uint32_t m_rows = 0;
    uint32_t m_columns = 0;

    void* m_imageData;
    int32_t m_fullWidth = 0;
    int32_t m_fullHeight = 0;
    int32_t m_channelCount = 0;
    uint32_t m_frameWidth = 0;
    uint32_t m_frameHeight = 0;
    uint32_t m_frameCount = 0;

    std::string m_name = "";
    GLuint m_arrayHandle;
};