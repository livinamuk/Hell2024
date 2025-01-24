#include "FlipbookTexture.h"
#include <iostream>
#include "Util.hpp"
#include "../API/OpenGL/GL_util.hpp"
#include "stb_image.h"

void FlipbookTexture::Load(const std::string& filepath) {

    FileInfo fileInfo = Util::GetFileInfoFromPath(filepath);

    size_t lastUnderscore = fileInfo.name.find_last_of('_');
    m_name = fileInfo.name;
    size_t dot = fileInfo.name.find_last_of('.');
    std::string dims = fileInfo.name.substr(lastUnderscore + 1, dot - lastUnderscore - 1);
    size_t x = dims.find('x');
    m_rows = std::stoi(dims.substr(0, x));
    m_columns = std::stoi(dims.substr(x + 1));

    stbi_set_flip_vertically_on_load(false);
    m_imageData = stbi_load(filepath.data(), &m_fullWidth, &m_fullHeight, &m_channelCount, 0);

    m_frameWidth = m_fullWidth / m_columns;
    m_frameHeight = m_fullHeight / m_rows;
    m_frameCount = m_rows * m_columns;

    GLint format = OpenGLUtil::GetFormatFromChannelCount(m_channelCount);
    GLint internalFormat = OpenGLUtil::GetInternalFormatFromChannelCount(m_channelCount);

    glGenTextures(1, &m_arrayHandle);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_arrayHandle);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, internalFormat, m_frameWidth, m_frameHeight, m_frameCount);
    unsigned char* data = static_cast<unsigned char*>(m_imageData);
    for (int row = 0; row < m_rows; ++row) {
        for (int col = 0; col < m_columns; ++col) {
            int layerIndex = row * m_columns + col;
            int offsetX = col * m_frameWidth;
            int offsetY = row * m_frameHeight;
            std::vector<unsigned char> layerData(m_frameWidth * m_frameHeight * m_channelCount);
            for (int y = 0; y < m_frameWidth; ++y) {
                for (int x = 0; x < m_frameWidth; ++x) {
                    for (int c = 0; c < m_channelCount; ++c) {
                        int srcIndex = ((offsetY + y) * m_fullWidth + (offsetX + x)) * m_channelCount + c;
                        int dstIndex = (y * m_frameWidth + x) * m_channelCount + c;
                        layerData[dstIndex] = data[srcIndex];
                    }
                }
            }
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, layerIndex, m_frameWidth, m_frameHeight, 1, format, GL_UNSIGNED_BYTE, layerData.data());
        }
    }
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}