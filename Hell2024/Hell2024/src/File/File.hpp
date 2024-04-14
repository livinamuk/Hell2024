#pragma once

#include <glad/glad.h>
#include "../../vendor/DDS/DDS_Helpers.h"

namespace File {

    uint32_t CMPToOpenGlFormat(CMP_FORMAT format) {
        if (format == CMP_FORMAT_DXT1) {
            return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        }
        else if (format == CMP_FORMAT_DXT3) {
            return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        }
        else if (format == CMP_FORMAT_DXT5) {
            return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        }
        else {
            return 0xFFFFFFFF;
        }
    }

    void FreeCMPTexture(CMP_Texture* t) {
        free(t->pData);
    }

}