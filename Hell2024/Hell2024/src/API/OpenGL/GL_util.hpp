#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "../../vendor/DDS/DDS_Helpers.h"
#include <string>

namespace OpenGLUtil {

    inline bool ExtensionExists(const std::string& extensionName) {
        static std::vector<std::string> extensionsCache;
        if (extensionsCache.empty()) {
            GLint numExtensions;
            glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
            for (GLint i = 0; i < numExtensions; ++i) {
                const char* extension = (const char*)glGetStringi(GL_EXTENSIONS, i);
                extensionsCache.push_back(extension);
            }
        }
        for (const auto& ext : extensionsCache) {
            if (ext == extensionName) {
                return true;
            }
        }
        return false;
    }

    inline uint32_t CMPFormatToGLInternalFromat(CMP_FORMAT format) {
        switch (format) {
        case CMP_FORMAT_DXT1:
            return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        case CMP_FORMAT_DXT3:
            return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        case CMP_FORMAT_DXT5:
            return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        case CMP_FORMAT_BC4:
            return GL_COMPRESSED_RED_RGTC1; // Single-channel compressed format
        case CMP_FORMAT_BC5:
            return GL_COMPRESSED_RG_RGTC2;  // Two-channel compressed format
        case CMP_FORMAT_ATI2N_XY:
            return GL_COMPRESSED_RG_RGTC2;  // Two-channel compressed format
        case CMP_FORMAT_BC6H:
            return GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT; // HDR format
        case CMP_FORMAT_BC7:
            return GL_COMPRESSED_RGBA_BPTC_UNORM;         // High-quality RGBA
        case CMP_FORMAT_ETC2_RGB:
            return GL_COMPRESSED_RGB8_ETC2;
        case CMP_FORMAT_ETC2_SRGB:
            return GL_COMPRESSED_SRGB8_ETC2;
        case CMP_FORMAT_ETC2_RGBA:
            return GL_COMPRESSED_RGBA8_ETC2_EAC;
        case CMP_FORMAT_ETC2_SRGBA:
            return GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC;
        case CMP_FORMAT_ASTC:
            return GL_COMPRESSED_RGBA_ASTC_4x4_KHR; // Assuming ASTC 4x4 block size
        case CMP_FORMAT_BC1:
            return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        case CMP_FORMAT_BC2:
            return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        default:
            return 0xFFFFFFFF; // Invalid format
        }
    }

    inline const char* GetGLFormatString(GLenum format) {
        switch (format) {
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
            return "GL_COMPRESSED_RGB_S3TC_DXT1_EXT";
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            return "GL_COMPRESSED_RGBA_S3TC_DXT1_EXT";
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
            return "GL_COMPRESSED_RGBA_S3TC_DXT3_EXT";
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            return "GL_COMPRESSED_RGBA_S3TC_DXT5_EXT";
        case GL_COMPRESSED_RED_RGTC1:
            return "GL_COMPRESSED_RED_RGTC1";
        case GL_COMPRESSED_SIGNED_RED_RGTC1:
            return "GL_COMPRESSED_SIGNED_RED_RGTC1";
        case GL_COMPRESSED_RG_RGTC2:
            return "GL_COMPRESSED_RG_RGTC2";
        case GL_COMPRESSED_SIGNED_RG_RGTC2:
            return "GL_COMPRESSED_SIGNED_RG_RGTC2";
        case GL_COMPRESSED_RGB8_ETC2:
            return "GL_COMPRESSED_RGB8_ETC2";
        case GL_COMPRESSED_SRGB8_ETC2:
            return "GL_COMPRESSED_SRGB8_ETC2";
        case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
            return "GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2";
        case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
            return "GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2";
        case GL_COMPRESSED_RGBA8_ETC2_EAC:
            return "GL_COMPRESSED_RGBA8_ETC2_EAC";
        case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
            return "GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC";
        default:
            return "Unknown Format";
        }
    }
}