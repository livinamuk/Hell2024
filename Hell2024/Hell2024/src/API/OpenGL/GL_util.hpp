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

    inline GLint GetFormatFromChannelCount(int channelCount) {
        switch (channelCount) {
        case 4:  return GL_RGBA;
        case 3:  return GL_RGB;
        case 1:  return GL_RED;
        default:
            std::cout << "Unsupported channel count: " << channelCount << "\n";
            return -1;
        }
    }

    inline GLint GetInternalFormatFromChannelCount(int channelCount) {
        switch (channelCount) {
        case 4:  return GL_RGBA8;
        case 3:  return GL_RGB8;
        case 1:  return GL_R8;
        default:
            std::cout << "Unsupported channel count: " << channelCount << "\n";
            return -1;
        }
    }

    inline uint32_t CMPFormatToGLFormat(CMP_FORMAT format) {
        switch (format) {
        case CMP_FORMAT_DXT1:
        case CMP_FORMAT_BC1:
        case CMP_FORMAT_ETC2_RGB:
        case CMP_FORMAT_ETC2_SRGB:
        case CMP_FORMAT_BC6H:
            return GL_RGB; // These formats are RGB-based

        case CMP_FORMAT_DXT3:
        case CMP_FORMAT_DXT5:
        case CMP_FORMAT_BC2:
        case CMP_FORMAT_BC7:
        case CMP_FORMAT_ETC2_RGBA:
        case CMP_FORMAT_ETC2_SRGBA:
        case CMP_FORMAT_ASTC:
            return GL_RGBA; // These formats are RGBA-based

        case CMP_FORMAT_BC4:
        case CMP_FORMAT_ATI2N_XY:
            return GL_RED; // These formats are single-channel (Red)

        case CMP_FORMAT_BC5:
            return GL_RG; // These formats are two-channel (Red-Green)

        default:
            return 0xFFFFFFFF; // Invalid format
        }
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

    inline const char* GetGLSyncStatusString(GLenum result) {
        switch (result) {
        case GL_ALREADY_SIGNALED:
            return "GL_ALREADY_SIGNALED";
        case GL_CONDITION_SATISFIED:
            return "GL_CONDITION_SATISFIED";
        case GL_TIMEOUT_EXPIRED:
            return "GL_TIMEOUT_EXPIRED";
        case GL_WAIT_FAILED:
            return "GL_WAIT_FAILED";
        default:
            return "UNKNOWN_GL_SYNC_STATUS";
        }
    }

    inline const char* GetGLFormatAsString(GLenum format) {
        switch (format) {
            // Compressed Texture Formats
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

            // Basic Color Formats
        case GL_RED:
            return "GL_RED";
        case GL_RG:
            return "GL_RG";
        case GL_RGB:
            return "GL_RGB";
        case GL_BGR:
            return "GL_BGR";
        case GL_RGBA:
            return "GL_RGBA";
        case GL_BGRA:
            return "GL_BGRA";

            // Unsigned Integer Formats
        case GL_R8:
            return "GL_R8";
        case GL_RG8:
            return "GL_RG8";
        case GL_RGB8:
            return "GL_RGB8";
        case GL_RGBA8:
            return "GL_RGBA8";
        case GL_SRGB8:
            return "GL_SRGB8";
        case GL_SRGB8_ALPHA8:
            return "GL_SRGB8_ALPHA8";

            // Signed Integer Formats
        case GL_R8I:
            return "GL_R8I";
        case GL_RG8I:
            return "GL_RG8I";
        case GL_RGB8I:
            return "GL_RGB8I";
        case GL_RGBA8I:
            return "GL_RGBA8I";

            // Floating-Point Formats
        case GL_R16F:
            return "GL_R16F";
        case GL_RG16F:
            return "GL_RG16F";
        case GL_RGB16F:
            return "GL_RGB16F";
        case GL_RGBA16F:
            return "GL_RGBA16F";
        case GL_R32F:
            return "GL_R32F";
        case GL_RG32F:
            return "GL_RG32F";
        case GL_RGB32F:
            return "GL_RGB32F";
        case GL_RGBA32F:
            return "GL_RGBA32F";

            // Depth and Stencil Formats
        case GL_DEPTH_COMPONENT:
            return "GL_DEPTH_COMPONENT";
        case GL_DEPTH_STENCIL:
            return "GL_DEPTH_STENCIL";
        case GL_DEPTH_COMPONENT16:
            return "GL_DEPTH_COMPONENT16";
        case GL_DEPTH_COMPONENT24:
            return "GL_DEPTH_COMPONENT24";
        case GL_DEPTH_COMPONENT32:
            return "GL_DEPTH_COMPONENT32";
        case GL_DEPTH_COMPONENT32F:
            return "GL_DEPTH_COMPONENT32F";
        case GL_DEPTH24_STENCIL8:
            return "GL_DEPTH24_STENCIL8";
        case GL_DEPTH32F_STENCIL8:
            return "GL_DEPTH32F_STENCIL8";

            // Other Specialized Formats
        case GL_R11F_G11F_B10F:
            return "GL_R11F_G11F_B10F";
        case GL_RGB9_E5:
            return "GL_RGB9_E5";
        case GL_R8_SNORM:
            return "GL_R8_SNORM";
        case GL_RG8_SNORM:
            return "GL_RG8_SNORM";
        case GL_RGB8_SNORM:
            return "GL_RGB8_SNORM";
        case GL_RGBA8_SNORM:
            return "GL_RGBA8_SNORM";
        case GL_R16_SNORM:
            return "GL_R16_SNORM";
        case GL_RG16_SNORM:
            return "GL_RG16_SNORM";
        case GL_RGB16_SNORM:
            return "GL_RGB16_SNORM";
        case GL_RGBA16_SNORM:
            return "GL_RGBA16_SNORM";

        default:
            return "Unknown Format";
        }
    }

    inline const char* GetGLInternalFormatAsString(GLenum internalFormat) {
        switch (internalFormat) {
            // Compressed Texture Formats
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

            // Basic Internal Formats
        case GL_R8:
            return "GL_R8";
        case GL_RG8:
            return "GL_RG8";
        case GL_RGB8:
            return "GL_RGB8";
        case GL_RGBA8:
            return "GL_RGBA8";
        case GL_SRGB8:
            return "GL_SRGB8";
        case GL_SRGB8_ALPHA8:
            return "GL_SRGB8_ALPHA8";
        case GL_R16F:
            return "GL_R16F";
        case GL_RG16F:
            return "GL_RG16F";
        case GL_RGB16F:
            return "GL_RGB16F";
        case GL_RGBA16F:
            return "GL_RGBA16F";
        case GL_R32F:
            return "GL_R32F";
        case GL_RG32F:
            return "GL_RG32F";
        case GL_RGB32F:
            return "GL_RGB32F";
        case GL_RGBA32F:
            return "GL_RGBA32F";

            // Depth and Stencil Formats
        case GL_DEPTH_COMPONENT:
            return "GL_DEPTH_COMPONENT";
        case GL_DEPTH_COMPONENT16:
            return "GL_DEPTH_COMPONENT16";
        case GL_DEPTH_COMPONENT24:
            return "GL_DEPTH_COMPONENT24";
        case GL_DEPTH_COMPONENT32:
            return "GL_DEPTH_COMPONENT32";
        case GL_DEPTH_COMPONENT32F:
            return "GL_DEPTH_COMPONENT32F";
        case GL_DEPTH24_STENCIL8:
            return "GL_DEPTH24_STENCIL8";
        case GL_DEPTH32F_STENCIL8:
            return "GL_DEPTH32F_STENCIL8";

            // Specialized Internal Formats
        case GL_R11F_G11F_B10F:
            return "GL_R11F_G11F_B10F";
        case GL_RGB9_E5:
            return "GL_RGB9_E5";
        case GL_R8_SNORM:
            return "GL_R8_SNORM";
        case GL_RG8_SNORM:
            return "GL_RG8_SNORM";
        case GL_RGB8_SNORM:
            return "GL_RGB8_SNORM";
        case GL_RGBA8_SNORM:
            return "GL_RGBA8_SNORM";
        case GL_R16_SNORM:
            return "GL_R16_SNORM";
        case GL_RG16_SNORM:
            return "GL_RG16_SNORM";
        case GL_RGB16_SNORM:
            return "GL_RGB16_SNORM";
        case GL_RGBA16_SNORM:
            return "GL_RGBA16_SNORM";

        default:
            return "Unknown Internal Format";
        }
    }

    inline GLint GetChannelCountFromFormat(GLenum format) {
        switch (format) {
            // Compressed Texture Formats
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGB8_ETC2:
        case GL_COMPRESSED_SRGB8_ETC2:
            return 3; // RGB formats

        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        case GL_COMPRESSED_RGBA8_ETC2_EAC:
        case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
        case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
        case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
            return 4; // RGBA formats

        case GL_COMPRESSED_RED_RGTC1:
        case GL_COMPRESSED_SIGNED_RED_RGTC1:
        case GL_RED:
        case GL_R8:
        case GL_R8I:
        case GL_R16F:
        case GL_R32F:
        case GL_R8_SNORM:
        case GL_R16_SNORM:
            return 1; // Single-channel formats

        case GL_COMPRESSED_RG_RGTC2:
        case GL_COMPRESSED_SIGNED_RG_RGTC2:
        case GL_RG:
        case GL_RG8:
        case GL_RG8I:
        case GL_RG16F:
        case GL_RG32F:
        case GL_RG8_SNORM:
        case GL_RG16_SNORM:
            return 2; // Two-channel formats

        case GL_RGB:
        case GL_BGR:
        case GL_RGB8:
        case GL_RGB8I:
        case GL_RGB16F:
        case GL_RGB32F:
        case GL_SRGB8:
        case GL_RGB8_SNORM:
        case GL_RGB16_SNORM:
        case GL_R11F_G11F_B10F:
        case GL_RGB9_E5:
            return 3; // RGB formats

        case GL_RGBA:
        case GL_BGRA:
        case GL_RGBA8:
        case GL_RGBA8I:
        case GL_RGBA16F:
        case GL_RGBA32F:
        case GL_SRGB8_ALPHA8:
        case GL_RGBA8_SNORM:
        case GL_RGBA16_SNORM:
            return 4; // RGBA formats

            // Depth and Stencil Formats
        case GL_DEPTH_COMPONENT:
        case GL_DEPTH_COMPONENT16:
        case GL_DEPTH_COMPONENT24:
        case GL_DEPTH_COMPONENT32:
        case GL_DEPTH_COMPONENT32F:
            return 1; // Depth formats are single channel

        case GL_DEPTH_STENCIL:
        case GL_DEPTH24_STENCIL8:
        case GL_DEPTH32F_STENCIL8:
            return 2; // Depth-stencil formats use 2 channels

        default:
            return -1; // Unknown or unsupported format
        }
    }

    inline size_t CalculateCompressedDataSize(GLenum format, int width, int height) {
        int blockSize = (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT || format == GL_COMPRESSED_RGB_S3TC_DXT1_EXT) ? 8 : 16;
        int blocksWide = std::max(1, (width + 3) / 4);
        int blocksHigh = std::max(1, (height + 3) / 4);
        return blocksWide * blocksHigh * blockSize;
    }
}