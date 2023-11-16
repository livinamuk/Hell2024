// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved
// Copyright (c) 2004-2006    ATI Technologies Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// AMD_Compress_Test_Helpers.cpp

#pragma warning(disable:4996)   // Ignoring 'fopen': This function or variable may be unsafe
#pragma warning(disable: 6387)

#include "Compressonator.h"
#include "DDS_Helpers.h"
#include <assert.h>
#include <stdio.h>
#include <ctype.h>

using namespace CMP;

#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
            ((CMP_DWORD)(CMP_BYTE)(ch0) | ((CMP_DWORD)(CMP_BYTE)(ch1) << 8) |   \
            ((CMP_DWORD)(CMP_BYTE)(ch2) << 16) | ((CMP_DWORD)(CMP_BYTE)(ch3) << 24 ))

#define FOURCC_ATI1N                MAKEFOURCC('A', 'T', 'I', '1')
#define FOURCC_ATI2N                MAKEFOURCC('A', 'T', 'I', '2')
#define FOURCC_ATI2N_XY             MAKEFOURCC('A', '2', 'X', 'Y')
#define FOURCC_ATI2N_DXT5           MAKEFOURCC('A', '2', 'D', '5')
#define FOURCC_DXT5_xGBR            MAKEFOURCC('x', 'G', 'B', 'R')
#define FOURCC_DXT5_RxBG            MAKEFOURCC('R', 'x', 'B', 'G')
#define FOURCC_DXT5_RBxG            MAKEFOURCC('R', 'B', 'x', 'G')
#define FOURCC_DXT5_xRBG            MAKEFOURCC('x', 'R', 'B', 'G')
#define FOURCC_DXT5_RGxB            MAKEFOURCC('R', 'G', 'x', 'B')
#define FOURCC_DXT5_xGxR            MAKEFOURCC('x', 'G', 'x', 'R')
#define FOURCC_APC1                 MAKEFOURCC('A', 'P', 'C', '1')
#define FOURCC_APC2                 MAKEFOURCC('A', 'P', 'C', '2')
#define FOURCC_APC3                 MAKEFOURCC('A', 'P', 'C', '3')
#define FOURCC_APC4                 MAKEFOURCC('A', 'P', 'C', '4')
#define FOURCC_APC5                 MAKEFOURCC('A', 'P', 'C', '5')
#define FOURCC_APC6                 MAKEFOURCC('A', 'P', 'C', '6')
#define FOURCC_ATC_RGB              MAKEFOURCC('A', 'T', 'C', ' ')
#define FOURCC_ATC_RGBA_EXPLICIT    MAKEFOURCC('A', 'T', 'C', 'A')
#define FOURCC_ATC_RGBA_INTERP      MAKEFOURCC('A', 'T', 'C', 'I')
#define FOURCC_ETC_RGB              MAKEFOURCC('E', 'T', 'C', ' ')
#define FOURCC_BC1                  MAKEFOURCC('B', 'C', '1', ' ')
#define FOURCC_BC2                  MAKEFOURCC('B', 'C', '2', ' ')
#define FOURCC_BC3                  MAKEFOURCC('B', 'C', '3', ' ')
#define FOURCC_BC4                  MAKEFOURCC('B', 'C', '4', ' ')
#define FOURCC_BC4S                 MAKEFOURCC('B', 'C', '4', 'S')
#define FOURCC_BC4U                 MAKEFOURCC('B', 'C', '4', 'U')
#define FOURCC_BC5                  MAKEFOURCC('B', 'C', '5', ' ')
#define FOURCC_BC5S                 MAKEFOURCC('B', 'C', '5', 'S')


// Deprecated but still supported for decompression
#define FOURCC_DXT5_GXRB            MAKEFOURCC('G', 'X', 'R', 'B')
#define FOURCC_DXT5_GRXB            MAKEFOURCC('G', 'R', 'X', 'B')
#define FOURCC_DXT5_RXGB            MAKEFOURCC('R', 'X', 'G', 'B')
#define FOURCC_DXT5_BRGX            MAKEFOURCC('B', 'R', 'G', 'X')

#define FOURCC_DXT1  (MAKEFOURCC('D','X','T','1'))
#define FOURCC_DXT2  (MAKEFOURCC('D','X','T','2'))
#define FOURCC_DXT3  (MAKEFOURCC('D','X','T','3'))
#define FOURCC_DXT4  (MAKEFOURCC('D','X','T','4'))
#define FOURCC_DXT5  (MAKEFOURCC('D','X','T','5'))
#define FOURCC_DX10  (MAKEFOURCC('D','X','1','0'))


typedef struct {
    CMP_DWORD dwFourCC;
    CMP_FORMAT nFormat;
} CMP_FourCC;

CMP_FourCC g_FourCCs[] = {
    {FOURCC_DXT1,               CMP_FORMAT_DXT1},
    {FOURCC_DXT3,               CMP_FORMAT_DXT3},
    {FOURCC_DXT5,               CMP_FORMAT_DXT5},
    {FOURCC_DXT5_xGBR,          CMP_FORMAT_DXT5_xGBR},
    {FOURCC_DXT5_RxBG,          CMP_FORMAT_DXT5_RxBG},
    {FOURCC_DXT5_RBxG,          CMP_FORMAT_DXT5_RBxG},
    {FOURCC_DXT5_xRBG,          CMP_FORMAT_DXT5_xRBG},
    {FOURCC_DXT5_RGxB,          CMP_FORMAT_DXT5_RGxB},
    {FOURCC_DXT5_xGxR,          CMP_FORMAT_DXT5_xGxR},
    {FOURCC_DXT5_GXRB,          CMP_FORMAT_DXT5_xRBG},
    {FOURCC_DXT5_GRXB,          CMP_FORMAT_DXT5_RxBG},
    {FOURCC_DXT5_RXGB,          CMP_FORMAT_DXT5_xGBR},
    {FOURCC_DXT5_BRGX,          CMP_FORMAT_DXT5_RGxB},
    {FOURCC_ATI1N,              CMP_FORMAT_ATI1N},
    {FOURCC_ATI2N,              CMP_FORMAT_ATI2N},
    {FOURCC_ATI2N_XY,           CMP_FORMAT_ATI2N_XY},
    {FOURCC_ATI2N_DXT5,         CMP_FORMAT_ATI2N_DXT5},
    {FOURCC_BC1,                CMP_FORMAT_BC1},
    {FOURCC_BC2,                CMP_FORMAT_BC2},
    {FOURCC_BC3,                CMP_FORMAT_BC3},
    {FOURCC_BC4,                CMP_FORMAT_BC4},
    {FOURCC_BC4S,               CMP_FORMAT_BC4},
    {FOURCC_BC4U,               CMP_FORMAT_BC4},
    {FOURCC_BC5,                CMP_FORMAT_BC5},
    {FOURCC_BC5S,               CMP_FORMAT_BC5},
    {FOURCC_ATC_RGB,            CMP_FORMAT_ATC_RGB},
    {FOURCC_ATC_RGBA_EXPLICIT,  CMP_FORMAT_ATC_RGBA_Explicit},
    {FOURCC_ATC_RGBA_INTERP,    CMP_FORMAT_ATC_RGBA_Interpolated},
    {FOURCC_ETC_RGB,            CMP_FORMAT_ETC_RGB},
};
CMP_DWORD g_dwFourCCCount = sizeof(g_FourCCs) / sizeof(g_FourCCs[0]);

CMP_FORMAT GetFormat(CMP_DWORD dwFourCC) {
    for(CMP_DWORD i = 0; i < g_dwFourCCCount; i++)
        if(g_FourCCs[i].dwFourCC == dwFourCC)
            return g_FourCCs[i].nFormat;

    return CMP_FORMAT_Unknown;
}

DWORD GetFourCC(CMP_FORMAT nFormat) {
    for(DWORD i = 0; i < g_dwFourCCCount; i++)
        if(g_FourCCs[i].nFormat == nFormat)
            return g_FourCCs[i].dwFourCC;

    return 0;
}

bool IsDXT5SwizzledFormat(CMP_FORMAT nFormat) {
    if(nFormat == CMP_FORMAT_DXT5_xGBR || nFormat == CMP_FORMAT_DXT5_RxBG || nFormat == CMP_FORMAT_DXT5_RBxG ||
            nFormat == CMP_FORMAT_DXT5_xRBG || nFormat == CMP_FORMAT_DXT5_RGxB || nFormat == CMP_FORMAT_DXT5_xGxR ||
            nFormat == CMP_FORMAT_ATI2N_DXT5)
        return true;
    else
        return false;
}

typedef struct {
    CMP_FORMAT nFormat;
    const char* pszFormatDesc;
} CMP_FormatDesc;

CMP_FormatDesc g_DDSFormatDesc[] = {
    {CMP_FORMAT_Unknown,                 ("Unknown")},
    {CMP_FORMAT_ARGB_8888,               ("ARGB_8888")},
    {CMP_FORMAT_BGRA_8888,               ("BGRA_8888")},
    {CMP_FORMAT_RGBA_8888,               ("RBGA_8888")},
    {CMP_FORMAT_RGB_888,                 ("RGB_888")},
    {CMP_FORMAT_BGR_888,                 ("BRG_888")},
    {CMP_FORMAT_RG_8,                    ("RG_8")},
    {CMP_FORMAT_R_8,                     ("R_8")},
    {CMP_FORMAT_ARGB_2101010,            ("ARGB_2101010")},
    {CMP_FORMAT_ARGB_16,                 ("ARGB_16")},
    {CMP_FORMAT_RG_16,                   ("RG_16")},
    {CMP_FORMAT_R_16,                    ("R_16")},
    {CMP_FORMAT_ARGB_16F,                ("ARGB_16F")},
    {CMP_FORMAT_RG_16F,                  ("RG_16F")},
    {CMP_FORMAT_R_16F,                   ("R_16F")},
    {CMP_FORMAT_ARGB_32F,                ("ARGB_32F")},
    {CMP_FORMAT_RG_32F,                  ("RG_32F")},
    {CMP_FORMAT_R_32F,                   ("R_32F")},
    {CMP_FORMAT_DXT1,                    ("DXT1")},
    {CMP_FORMAT_DXT3,                    ("DXT3")},
    {CMP_FORMAT_DXT5,                    ("DXT5")},
    {CMP_FORMAT_DXT5_xGBR,               ("DXT5_xGBR")},
    {CMP_FORMAT_DXT5_RxBG,               ("DXT5_RxBG")},
    {CMP_FORMAT_DXT5_RBxG,               ("DXT5_RBxG")},
    {CMP_FORMAT_DXT5_xRBG,               ("DXT5_xRBG")},
    {CMP_FORMAT_DXT5_RGxB,               ("DXT5_RGxB")},
    {CMP_FORMAT_DXT5_xGxR,               ("DXT5_xGxR")},
    {CMP_FORMAT_ATI1N,                   ("ATI1N")},
    {CMP_FORMAT_ATI2N,                   ("ATI2N")},
    {CMP_FORMAT_ATI2N_XY,                ("ATI2N_XY")},
    {CMP_FORMAT_ATI2N_DXT5,              ("ATI2N_DXT5")},
    {CMP_FORMAT_BC1,                     ("BC1")},
    {CMP_FORMAT_BC2,                     ("BC2")},
    {CMP_FORMAT_BC3,                     ("BC3")},
    {CMP_FORMAT_BC4,                     ("BC4")},
    {CMP_FORMAT_BC5,                     ("BC5")},
    {CMP_FORMAT_BC6H,                    ("BC6H") },
    {CMP_FORMAT_BC7,                     ("BC7") },
    {CMP_FORMAT_ATC_RGB,                 ("ATC_RGB")},
    {CMP_FORMAT_ATC_RGBA_Explicit,       ("ATC_RGBA_Explicit")},
    {CMP_FORMAT_ATC_RGBA_Interpolated,   ("ATC_RGBA_Interpolated")},
    {CMP_FORMAT_ETC_RGB,                 ("ETC_RGB")},
};

DWORD g_dwDDSFormatDescCount = sizeof(g_DDSFormatDesc) / sizeof(g_DDSFormatDesc[0]);

static bool stringicmp( const char *left, const char *right ) {
    while ( true ) {
        char leftc = *left;
        char rightc = *right;

        if ( !leftc && !rightc ) {
            return true;
        }

        if ( !leftc || !rightc ) {
            return false;
        }

        if ( toupper( leftc ) != toupper( rightc ) ) {
            return false;
        }

        left++;
        right++;
    }

    return false;
}

CMP_FORMAT ParseFormat(const char* pszFormat) {
    if(pszFormat == NULL)
        return CMP_FORMAT_Unknown;

    for(DWORD i = 0; i < g_dwDDSFormatDescCount; i++)
        if(stringicmp(pszFormat,g_DDSFormatDesc[i].pszFormatDesc))
            return g_DDSFormatDesc[i].nFormat;

    return CMP_FORMAT_Unknown;
}

const char* GetFormatDescription(CMP_FORMAT nFormat) {
    for(DWORD i = 0; i < g_dwDDSFormatDescCount; i++)
        if(nFormat == g_DDSFormatDesc[i].nFormat)
            return g_DDSFormatDesc[i].pszFormatDesc;

    return g_DDSFormatDesc[0].pszFormatDesc;
}

#define POINTER_64 __ptr64

#pragma pack(4)

typedef struct _DDCOLORKEY {
    DWORD       dwColorSpaceLowValue;   // low boundary of color space that is to
    // be treated as Color Key, inclusive
    DWORD       dwColorSpaceHighValue;  // high boundary of color space that is
    // to be treated as Color Key, inclusive
} DDCOLORKEY;

typedef struct _DDPIXELFORMAT {
    DWORD       dwSize;                 // size of structure
    DWORD       dwFlags;                // pixel format flags
    DWORD       dwFourCC;               // (FOURCC code)
    union {
        DWORD   dwRGBBitCount;          // how many bits per pixel
        DWORD   dwYUVBitCount;          // how many bits per pixel
        DWORD   dwZBufferBitDepth;      // how many total bits/pixel in z buffer (including any stencil bits)
        DWORD   dwAlphaBitDepth;        // how many bits for alpha channels
        DWORD   dwLuminanceBitCount;    // how many bits per pixel
        DWORD   dwBumpBitCount;         // how many bits per "buxel", total
        DWORD   dwPrivateFormatBitCount;// Bits per pixel of private driver formats. Only valid in texture
        // format list and if DDPF_D3DFORMAT is set
    };
    union {
        DWORD   dwRBitMask;             // mask for red bit
        DWORD   dwYBitMask;             // mask for Y bits
        DWORD   dwStencilBitDepth;      // how many stencil bits (note: dwZBufferBitDepth-dwStencilBitDepth is total Z-only bits)
        DWORD   dwLuminanceBitMask;     // mask for luminance bits
        DWORD   dwBumpDuBitMask;        // mask for bump map U delta bits
        DWORD   dwOperations;           // DDPF_D3DFORMAT Operations
    };
    union {
        DWORD   dwGBitMask;             // mask for green bits
        DWORD   dwUBitMask;             // mask for U bits
        DWORD   dwZBitMask;             // mask for Z bits
        DWORD   dwBumpDvBitMask;        // mask for bump map V delta bits
        struct {
            WORD    wFlipMSTypes;       // Multisample methods supported via flip for this D3DFORMAT
            WORD    wBltMSTypes;        // Multisample methods supported via blt for this D3DFORMAT
        } MultiSampleCaps;

    };
    union {
        DWORD   dwBBitMask;             // mask for blue bits
        DWORD   dwVBitMask;             // mask for V bits
        DWORD   dwStencilBitMask;       // mask for stencil bits
        DWORD   dwBumpLuminanceBitMask; // mask for luminance in bump map
    };
    union {
        DWORD   dwRGBAlphaBitMask;      // mask for alpha channel
        DWORD   dwYUVAlphaBitMask;      // mask for alpha channel
        DWORD   dwLuminanceAlphaBitMask;// mask for alpha channel
        DWORD   dwRGBZBitMask;          // mask for Z channel
        DWORD   dwYUVZBitMask;          // mask for Z channel
    };
} DDPIXELFORMAT;

typedef struct _DDSCAPS2 {
    DWORD       dwCaps;         // capabilities of surface wanted
    DWORD       dwCaps2;
    DWORD       dwCaps3;
    union {
        DWORD       dwCaps4;
        DWORD       dwVolumeDepth;
    };
} DDSCAPS2;

typedef struct _DDSURFACEDESC2_64 {
    DWORD               dwSize;                 // size of the DDSURFACEDESC structure
    DWORD               dwFlags;                // determines what fields are valid
    DWORD               dwHeight;               // height of surface to be created
    DWORD               dwWidth;                // width of input surface
    union {
        LONG            lPitch;                 // distance to start of next line (return value only)
        DWORD           dwLinearSize;           // Formless late-allocated optimized surface size
    };
    union {
        DWORD           dwBackBufferCount;      // number of back buffers requested
        DWORD           dwDepth;                // the depth if this is a volume texture
    };
    union {
        DWORD           dwMipMapCount;          // number of mip-map levels requestde
        // dwZBufferBitDepth removed, use ddpfPixelFormat one instead
        DWORD           dwRefreshRate;          // refresh rate (used when display mode is described)
        DWORD           dwSrcVBHandle;          // The source used in VB::Optimize
    };
    DWORD               dwAlphaBitDepth;        // depth of alpha buffer requested
    DWORD               dwReserved;             // reserved
    DWORD               lpSurface;              // pointer to the associated surface memory
    union {
        DDCOLORKEY      ddckCKDestOverlay;      // color key for destination overlay use
        DWORD           dwEmptyFaceColor;       // Physical color for empty cubemap faces
    };
    DDCOLORKEY          ddckCKDestBlt;          // color key for destination blt use
    DDCOLORKEY          ddckCKSrcOverlay;       // color key for source overlay use
    DDCOLORKEY          ddckCKSrcBlt;           // color key for source blt use
    union {
        DDPIXELFORMAT   ddpfPixelFormat;        // pixel format description of the surface
        DWORD           dwFVF;                  // vertex format description of vertex buffers
    };
    DDSCAPS2            ddsCaps;                // direct draw surface capabilities
    DWORD               dwTextureStage;         // stage in multitexture cascade
} DDSURFACEDESC2;

#define DDSD2 DDSURFACEDESC2

static const DWORD DDS_HEADER = MAKEFOURCC('D', 'D', 'S', ' ');

bool LoadDDSFile(const char* pszFile, CMP_Texture& texture) {
    FILE* pSourceFile = fopen(pszFile, ("rb"));

    if (!pSourceFile) return false;

    DWORD dwFileHeader;
    fread(&dwFileHeader, sizeof(DWORD), 1, pSourceFile);
    if(dwFileHeader != DDS_HEADER) {
        printf("Source file is not a valid DDS.\n");
        fclose(pSourceFile);
        return false;
    }

    DDSD2 ddsd;
    fread(&ddsd, sizeof(DDSD2), 1, pSourceFile);

    memset(&texture, 0, sizeof(texture));
    texture.dwSize = sizeof(texture);
    texture.dwWidth = ddsd.dwWidth;
    texture.dwHeight = ddsd.dwHeight;
    texture.dwPitch = ddsd.lPitch;

    if(ddsd.ddpfPixelFormat.dwRGBBitCount==32)
    {
        // Visual Studio reports as 32bpp RGBA
        if ((ddsd.ddpfPixelFormat.dwRBitMask         == 0x000000ff)&&
            (ddsd.ddpfPixelFormat.dwGBitMask         == 0x0000ff00)&&
            (ddsd.ddpfPixelFormat.dwBBitMask         == 0x00ff0000)&&
            (ddsd.ddpfPixelFormat.dwRGBAlphaBitMask  == 0xff000000))
        {
            texture.format = CMP_FORMAT_RGBA_8888;
        } 
        else 
            // Visual Studio reports as 32bpp BGRA
        if ((ddsd.ddpfPixelFormat.dwRBitMask        == 0x00ff0000) &&
            (ddsd.ddpfPixelFormat.dwGBitMask        == 0x0000ff00) &&
            (ddsd.ddpfPixelFormat.dwBBitMask        == 0x000000ff) &&
            (ddsd.ddpfPixelFormat.dwRGBAlphaBitMask == 0xff000000))
        {
            texture.format = CMP_FORMAT_BGRA_8888;
        }
    }
    else if(ddsd.ddpfPixelFormat.dwRGBBitCount==24)
    {
        // assumptions is made here should check all channel locations
        if (ddsd.ddpfPixelFormat.dwRBitMask == 0xff) {
            texture.format = CMP_FORMAT_RGB_888;
        }
        else {
            texture.format = CMP_FORMAT_BGR_888;
        }
    }
    else if(GetFormat(ddsd.ddpfPixelFormat.dwPrivateFormatBitCount) != CMP_FORMAT_Unknown)
        texture.format = GetFormat(ddsd.ddpfPixelFormat.dwPrivateFormatBitCount);
    else if(GetFormat(ddsd.ddpfPixelFormat.dwFourCC) != CMP_FORMAT_Unknown)
        texture.format = GetFormat(ddsd.ddpfPixelFormat.dwFourCC);
    else {
        printf("Unsupported source format.\n");
        fclose(pSourceFile);
        return false;
    }

    // Init source texture
    texture.dwDataSize = CMP_CalculateBufferSize(&texture);
    texture.pData = (CMP_BYTE*) malloc(texture.dwDataSize);

    fread(texture.pData, texture.dwDataSize, 1, pSourceFile);
    fclose(pSourceFile);

    return true;
}

enum D3D10_RESOURCE_DIMENSION {
    D3D10_RESOURCE_DIMENSION_UNKNOWN	= 0,
    D3D10_RESOURCE_DIMENSION_BUFFER	= 1,
    D3D10_RESOURCE_DIMENSION_TEXTURE1D	= 2,
    D3D10_RESOURCE_DIMENSION_TEXTURE2D	= 3,
    D3D10_RESOURCE_DIMENSION_TEXTURE3D	= 4
};

typedef struct {
    DWORD                           dxgiFormat;
    D3D10_RESOURCE_DIMENSION        resourceDimension;
    UINT                            miscFlag;                   // Used for D3D10_RESOURCE_MISC_FLAG
    UINT                            arraySize;
    UINT                            reserved;                   // Currently unused
} DDS_HEADER_DDS10;

#define DDSD_CAPS               0x00000001l     // default
#define DDSD_HEIGHT             0x00000002l
#define DDSD_WIDTH              0x00000004l
#define DDSD_PITCH              0x00000008l
#define DDSD_BACKBUFFERCOUNT    0x00000020l
#define DDSD_ZBUFFERBITDEPTH    0x00000040l
#define DDSD_ALPHABITDEPTH      0x00000080l
#define DDSD_LPSURFACE          0x00000800l
#define DDSD_PIXELFORMAT        0x00001000l
#define DDSD_CKDESTOVERLAY      0x00002000l
#define DDSD_CKDESTBLT          0x00004000l
#define DDSD_CKSRCOVERLAY       0x00008000l
#define DDSD_CKSRCBLT           0x00010000l
#define DDSD_MIPMAPCOUNT        0x00020000l
#define DDSD_REFRESHRATE        0x00040000l
#define DDSD_LINEARSIZE         0x00080000l
#define DDSD_TEXTURESTAGE       0x00100000l
#define DDSD_FVF                0x00200000l
#define DDSD_SRCVBHANDLE        0x00400000l
#define DDSD_DEPTH              0x00800000l
#define DDSD_ALL                0x00fff9eel

#define DDSCAPS_RESERVED1                       0x00000001l
#define DDSCAPS_ALPHA                           0x00000002l
#define DDSCAPS_BACKBUFFER                      0x00000004l
#define DDSCAPS_COMPLEX                         0x00000008l
#define DDSCAPS_FLIP                            0x00000010l
#define DDSCAPS_FRONTBUFFER                     0x00000020l
#define DDSCAPS_OFFSCREENPLAIN                  0x00000040l
#define DDSCAPS_OVERLAY                         0x00000080l
#define DDSCAPS_PALETTE                         0x00000100l
#define DDSCAPS_PRIMARYSURFACE                  0x00000200l
#define DDSCAPS_RESERVED3                       0x00000400l
#define DDSCAPS_PRIMARYSURFACELEFT              0x00000000l
#define DDSCAPS_SYSTEMMEMORY                    0x00000800l
#define DDSCAPS_TEXTURE                         0x00001000l
#define DDSCAPS_3DDEVICE                        0x00002000l
#define DDSCAPS_VIDEOMEMORY                     0x00004000l
#define DDSCAPS_VISIBLE                         0x00008000l
#define DDSCAPS_WRITEONLY                       0x00010000l
#define DDSCAPS_ZBUFFER                         0x00020000l
#define DDSCAPS_OWNDC                           0x00040000l
#define DDSCAPS_LIVEVIDEO                       0x00080000l
#define DDSCAPS_HWCODEC                         0x00100000l
#define DDSCAPS_MODEX                           0x00200000l
#define DDSCAPS_MIPMAP                          0x00400000l
#define DDSCAPS_RESERVED2                       0x00800000l
#define DDSCAPS_ALLOCONLOAD                     0x04000000l
#define DDSCAPS_VIDEOPORT                       0x08000000l
#define DDSCAPS_LOCALVIDMEM                     0x10000000l
#define DDSCAPS_NONLOCALVIDMEM                  0x20000000l
#define DDSCAPS_STANDARDVGAMODE                 0x40000000l
#define DDSCAPS_OPTIMIZED                       0x80000000l

#define DDSCAPS2_RESERVED4                      0x00000002L
#define DDSCAPS2_HARDWAREDEINTERLACE            0x00000000L
#define DDSCAPS2_HINTDYNAMIC                    0x00000004L
#define DDSCAPS2_HINTSTATIC                     0x00000008L
#define DDSCAPS2_TEXTUREMANAGE                  0x00000010L
#define DDSCAPS2_RESERVED1                      0x00000020L
#define DDSCAPS2_RESERVED2                      0x00000040L
#define DDSCAPS2_OPAQUE                         0x00000080L
#define DDSCAPS2_HINTANTIALIASING               0x00000100L
#define DDSCAPS2_CUBEMAP                        0x00000200L
#define DDSCAPS2_CUBEMAP_POSITIVEX              0x00000400L
#define DDSCAPS2_CUBEMAP_NEGATIVEX              0x00000800L
#define DDSCAPS2_CUBEMAP_POSITIVEY              0x00001000L
#define DDSCAPS2_CUBEMAP_NEGATIVEY              0x00002000L
#define DDSCAPS2_CUBEMAP_POSITIVEZ              0x00004000L
#define DDSCAPS2_CUBEMAP_NEGATIVEZ              0x00008000L
#define DDSCAPS2_CUBEMAP_ALLFACES ( DDSCAPS2_CUBEMAP_POSITIVEX |\
                                    DDSCAPS2_CUBEMAP_NEGATIVEX |\
                                    DDSCAPS2_CUBEMAP_POSITIVEY |\
                                    DDSCAPS2_CUBEMAP_NEGATIVEY |\
                                    DDSCAPS2_CUBEMAP_POSITIVEZ |\
                                    DDSCAPS2_CUBEMAP_NEGATIVEZ )

#define DDSCAPS2_MIPMAPSUBLEVEL                 0x00010000L
#define DDSCAPS2_D3DTEXTUREMANAGE               0x00020000L
#define DDSCAPS2_DONOTPERSIST                   0x00040000L
#define DDSCAPS2_STEREOSURFACELEFT              0x00080000L
#define DDSCAPS2_VOLUME                         0x00200000L
#define DDSCAPS2_NOTUSERLOCKABLE                0x00400000L
#define DDSCAPS2_POINTS                         0x00800000L
#define DDSCAPS2_RTPATCHES                      0x01000000L
#define DDSCAPS2_NPATCHES                       0x02000000L
#define DDSCAPS2_RESERVED3                      0x04000000L
#define DDSCAPS2_DISCARDBACKBUFFER              0x10000000L
#define DDSCAPS2_ENABLEALPHACHANNEL             0x20000000L
#define DDSCAPS2_EXTENDEDFORMATPRIMARY          0x40000000L
#define DDSCAPS2_ADDITIONALPRIMARY              0x80000000L
#define DDSCAPS3_MULTISAMPLE_MASK               0x0000001FL
#define DDSCAPS3_MULTISAMPLE_QUALITY_MASK       0x000000E0L
#define DDSCAPS3_MULTISAMPLE_QUALITY_SHIFT      5
#define DDSCAPS3_RESERVED1                      0x00000100L
#define DDSCAPS3_RESERVED2                      0x00000200L
#define DDSCAPS3_LIGHTWEIGHTMIPMAP              0x00000400L
#define DDSCAPS3_AUTOGENMIPMAP                  0x00000800L
#define DDSCAPS3_DMAP                           0x00001000L

#define DDPF_ALPHAPIXELS                        0x00000001l
#define DDPF_ALPHA                              0x00000002l
#define DDPF_FOURCC                             0x00000004l
#define DDPF_PALETTEINDEXED4                    0x00000008l
#define DDPF_PALETTEINDEXEDTO8                  0x00000010l
#define DDPF_PALETTEINDEXED8                    0x00000020l
#define DDPF_RGB                                0x00000040l
#define DDPF_COMPRESSED                         0x00000080l
#define DDPF_RGBTOYUV                           0x00000100l
#define DDPF_YUV                                0x00000200l
#define DDPF_ZBUFFER                            0x00000400l
#define DDPF_PALETTEINDEXED1                    0x00000800l
#define DDPF_PALETTEINDEXED2                    0x00001000l
#define DDPF_ZPIXELS                            0x00002000l
#define DDPF_STENCILBUFFER                      0x00004000l
#define DDPF_ALPHAPREMULT                       0x00008000l
#define DDPF_LUMINANCE                          0x00020000l
#define DDPF_BUMPLUMINANCE                      0x00040000l
#define DDPF_BUMPDUDV                           0x00080000l

typedef enum DXGI_FORMAT {
    DXGI_FORMAT_UNKNOWN	                    = 0,
    DXGI_FORMAT_R32G32B32A32_TYPELESS       = 1,
    DXGI_FORMAT_R32G32B32A32_FLOAT          = 2,
    DXGI_FORMAT_R32G32B32A32_UINT           = 3,
    DXGI_FORMAT_R32G32B32A32_SINT           = 4,
    DXGI_FORMAT_R32G32B32_TYPELESS          = 5,
    DXGI_FORMAT_R32G32B32_FLOAT             = 6,
    DXGI_FORMAT_R32G32B32_UINT              = 7,
    DXGI_FORMAT_R32G32B32_SINT              = 8,
    DXGI_FORMAT_R16G16B16A16_TYPELESS       = 9,
    DXGI_FORMAT_R16G16B16A16_FLOAT          = 10,
    DXGI_FORMAT_R16G16B16A16_UNORM          = 11,
    DXGI_FORMAT_R16G16B16A16_UINT           = 12,
    DXGI_FORMAT_R16G16B16A16_SNORM          = 13,
    DXGI_FORMAT_R16G16B16A16_SINT           = 14,
    DXGI_FORMAT_R32G32_TYPELESS             = 15,
    DXGI_FORMAT_R32G32_FLOAT                = 16,
    DXGI_FORMAT_R32G32_UINT                 = 17,
    DXGI_FORMAT_R32G32_SINT                 = 18,
    DXGI_FORMAT_R32G8X24_TYPELESS           = 19,
    DXGI_FORMAT_D32_FLOAT_S8X24_UINT        = 20,
    DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS    = 21,
    DXGI_FORMAT_X32_TYPELESS_G8X24_UINT     = 22,
    DXGI_FORMAT_R10G10B10A2_TYPELESS        = 23,
    DXGI_FORMAT_R10G10B10A2_UNORM           = 24,
    DXGI_FORMAT_R10G10B10A2_UINT            = 25,
    DXGI_FORMAT_R11G11B10_FLOAT             = 26,
    DXGI_FORMAT_R8G8B8A8_TYPELESS           = 27,
    DXGI_FORMAT_R8G8B8A8_UNORM              = 28,
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB         = 29,
    DXGI_FORMAT_R8G8B8A8_UINT               = 30,
    DXGI_FORMAT_R8G8B8A8_SNORM              = 31,
    DXGI_FORMAT_R8G8B8A8_SINT               = 32,
    DXGI_FORMAT_R16G16_TYPELESS             = 33,
    DXGI_FORMAT_R16G16_FLOAT                = 34,
    DXGI_FORMAT_R16G16_UNORM                = 35,
    DXGI_FORMAT_R16G16_UINT                 = 36,
    DXGI_FORMAT_R16G16_SNORM                = 37,
    DXGI_FORMAT_R16G16_SINT                 = 38,
    DXGI_FORMAT_R32_TYPELESS                = 39,
    DXGI_FORMAT_D32_FLOAT                   = 40,
    DXGI_FORMAT_R32_FLOAT                   = 41,
    DXGI_FORMAT_R32_UINT                    = 42,
    DXGI_FORMAT_R32_SINT                    = 43,
    DXGI_FORMAT_R24G8_TYPELESS              = 44,
    DXGI_FORMAT_D24_UNORM_S8_UINT           = 45,
    DXGI_FORMAT_R24_UNORM_X8_TYPELESS       = 46,
    DXGI_FORMAT_X24_TYPELESS_G8_UINT        = 47,
    DXGI_FORMAT_R8G8_TYPELESS               = 48,
    DXGI_FORMAT_R8G8_UNORM                  = 49,
    DXGI_FORMAT_R8G8_UINT                   = 50,
    DXGI_FORMAT_R8G8_SNORM                  = 51,
    DXGI_FORMAT_R8G8_SINT                   = 52,
    DXGI_FORMAT_R16_TYPELESS                = 53,
    DXGI_FORMAT_R16_FLOAT                   = 54,
    DXGI_FORMAT_D16_UNORM                   = 55,
    DXGI_FORMAT_R16_UNORM                   = 56,
    DXGI_FORMAT_R16_UINT                    = 57,
    DXGI_FORMAT_R16_SNORM                   = 58,
    DXGI_FORMAT_R16_SINT                    = 59,
    DXGI_FORMAT_R8_TYPELESS                 = 60,
    DXGI_FORMAT_R8_UNORM                    = 61,
    DXGI_FORMAT_R8_UINT                     = 62,
    DXGI_FORMAT_R8_SNORM                    = 63,
    DXGI_FORMAT_R8_SINT                     = 64,
    DXGI_FORMAT_A8_UNORM                    = 65,
    DXGI_FORMAT_R1_UNORM                    = 66,
    DXGI_FORMAT_R9G9B9E5_SHAREDEXP          = 67,
    DXGI_FORMAT_R8G8_B8G8_UNORM             = 68,
    DXGI_FORMAT_G8R8_G8B8_UNORM             = 69,
    DXGI_FORMAT_BC1_TYPELESS                = 70,
    DXGI_FORMAT_BC1_UNORM                   = 71,
    DXGI_FORMAT_BC1_UNORM_SRGB              = 72,
    DXGI_FORMAT_BC2_TYPELESS                = 73,
    DXGI_FORMAT_BC2_UNORM                   = 74,
    DXGI_FORMAT_BC2_UNORM_SRGB              = 75,
    DXGI_FORMAT_BC3_TYPELESS                = 76,
    DXGI_FORMAT_BC3_UNORM                   = 77,
    DXGI_FORMAT_BC3_UNORM_SRGB              = 78,
    DXGI_FORMAT_BC4_TYPELESS                = 79,
    DXGI_FORMAT_BC4_UNORM                   = 80,
    DXGI_FORMAT_BC4_SNORM                   = 81,
    DXGI_FORMAT_BC5_TYPELESS                = 82,
    DXGI_FORMAT_BC5_UNORM                   = 83,
    DXGI_FORMAT_BC5_SNORM                   = 84,
    DXGI_FORMAT_B5G6R5_UNORM                = 85,
    DXGI_FORMAT_B5G5R5A1_UNORM              = 86,
    DXGI_FORMAT_B8G8R8A8_UNORM              = 87,
    DXGI_FORMAT_B8G8R8X8_UNORM              = 88,
    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM  = 89,
    DXGI_FORMAT_B8G8R8A8_TYPELESS           = 90,
    DXGI_FORMAT_B8G8R8A8_UNORM_SRGB         = 91,
    DXGI_FORMAT_B8G8R8X8_TYPELESS           = 92,
    DXGI_FORMAT_B8G8R8X8_UNORM_SRGB         = 93,
    DXGI_FORMAT_BC6H_TYPELESS               = 94,
    DXGI_FORMAT_BC6H_UF16                   = 95,
    DXGI_FORMAT_BC6H_SF16                   = 96,
    DXGI_FORMAT_BC7_TYPELESS                = 97,
    DXGI_FORMAT_BC7_UNORM                   = 98,
    DXGI_FORMAT_BC7_UNORM_SRGB              = 99,
    DXGI_FORMAT_AYUV                        = 100,
    DXGI_FORMAT_Y410                        = 101,
    DXGI_FORMAT_Y416                        = 102,
    DXGI_FORMAT_NV12                        = 103,
    DXGI_FORMAT_P010                        = 104,
    DXGI_FORMAT_P016                        = 105,
    DXGI_FORMAT_420_OPAQUE                  = 106,
    DXGI_FORMAT_YUY2                        = 107,
    DXGI_FORMAT_Y210                        = 108,
    DXGI_FORMAT_Y216                        = 109,
    DXGI_FORMAT_NV11                        = 110,
    DXGI_FORMAT_AI44                        = 111,
    DXGI_FORMAT_IA44                        = 112,
    DXGI_FORMAT_P8                          = 113,
    DXGI_FORMAT_A8P8                        = 114,
    DXGI_FORMAT_B4G4R4A4_UNORM              = 115,
    DXGI_FORMAT_FORCE_UINT                  = 0xffffffff
} DXGI_FORMAT;

typedef enum _D3DFORMAT {
    D3DFMT_UNKNOWN              =  0,

    D3DFMT_R8G8B8               = 20,
    D3DFMT_A8R8G8B8             = 21,
    D3DFMT_X8R8G8B8             = 22,
    D3DFMT_R5G6B5               = 23,
    D3DFMT_X1R5G5B5             = 24,
    D3DFMT_A1R5G5B5             = 25,
    D3DFMT_A4R4G4B4             = 26,
    D3DFMT_R3G3B2               = 27,
    D3DFMT_A8                   = 28,
    D3DFMT_A8R3G3B2             = 29,
    D3DFMT_X4R4G4B4             = 30,
    D3DFMT_A2B10G10R10          = 31,
    D3DFMT_A8B8G8R8             = 32,
    D3DFMT_X8B8G8R8             = 33,
    D3DFMT_G16R16               = 34,
    D3DFMT_A2R10G10B10          = 35,
    D3DFMT_A16B16G16R16         = 36,

    D3DFMT_A8P8                 = 40,
    D3DFMT_P8                   = 41,

    D3DFMT_L8                   = 50,
    D3DFMT_A8L8                 = 51,
    D3DFMT_A4L4                 = 52,

    D3DFMT_V8U8                 = 60,
    D3DFMT_L6V5U5               = 61,
    D3DFMT_X8L8V8U8             = 62,
    D3DFMT_Q8W8V8U8             = 63,
    D3DFMT_V16U16               = 64,
    D3DFMT_A2W10V10U10          = 67,

    D3DFMT_UYVY                 = MAKEFOURCC('U', 'Y', 'V', 'Y'),
    D3DFMT_R8G8_B8G8            = MAKEFOURCC('R', 'G', 'B', 'G'),
    D3DFMT_YUY2                 = MAKEFOURCC('Y', 'U', 'Y', '2'),
    D3DFMT_G8R8_G8B8            = MAKEFOURCC('G', 'R', 'G', 'B'),
    D3DFMT_DXT1                 = MAKEFOURCC('D', 'X', 'T', '1'),
    D3DFMT_DXT2                 = MAKEFOURCC('D', 'X', 'T', '2'),
    D3DFMT_DXT3                 = MAKEFOURCC('D', 'X', 'T', '3'),
    D3DFMT_DXT4                 = MAKEFOURCC('D', 'X', 'T', '4'),
    D3DFMT_DXT5                 = MAKEFOURCC('D', 'X', 'T', '5'),

    D3DFMT_D16_LOCKABLE         = 70,
    D3DFMT_D32                  = 71,
    D3DFMT_D15S1                = 73,
    D3DFMT_D24S8                = 75,
    D3DFMT_D24X8                = 77,
    D3DFMT_D24X4S4              = 79,
    D3DFMT_D16                  = 80,

    D3DFMT_D32F_LOCKABLE        = 82,
    D3DFMT_D24FS8               = 83,

    D3DFMT_L16                  = 81,

    D3DFMT_VERTEXDATA           =100,
    D3DFMT_INDEX16              =101,
    D3DFMT_INDEX32              =102,

    D3DFMT_Q16W16V16U16         =110,

    D3DFMT_MULTI2_ARGB8         = MAKEFOURCC('M','E','T','1'),

    // Floating point surface formats

    // s10e5 formats (16-bits per channel)
    D3DFMT_R16F                 = 111,
    D3DFMT_G16R16F              = 112,
    D3DFMT_A16B16G16R16F        = 113,

    // IEEE s23e8 formats (32-bits per channel)
    D3DFMT_R32F                 = 114,
    D3DFMT_G32R32F              = 115,
    D3DFMT_A32B32G32R32F        = 116,

    D3DFMT_CxV8U8               = 117,
} D3DFORMAT;


void SaveDDSFile(const char* pszFile, CMP_Texture& texture) {
    FILE* pFile = fopen(pszFile, ("wb"));
    if(!pFile)
        return;

    bool DDSet = false;

    switch (texture.format) {
    case CMP_FORMAT_BC1:
    case CMP_FORMAT_BC2:
    case CMP_FORMAT_BC3:
    case CMP_FORMAT_BC4:
    case CMP_FORMAT_BC5: {
        fwrite(&DDS_HEADER, sizeof(DWORD), 1, pFile);
        DDSD2 ddsd2;
        memset(&ddsd2, 0, sizeof(DDSD2));
        ddsd2.dwSize                   = sizeof(DDSD2);
        ddsd2.dwWidth                  = texture.dwWidth;
        ddsd2.dwHeight                 = texture.dwHeight;
        ddsd2.dwMipMapCount            = 1;
        ddsd2.dwFlags                  = DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT|DDSD_MIPMAPCOUNT;
        ddsd2.dwFlags                 |= DDSD_LINEARSIZE;
        ddsd2.dwLinearSize             = texture.dwDataSize;
        ddsd2.ddpfPixelFormat.dwSize   = sizeof(DDPIXELFORMAT);
        ddsd2.ddsCaps.dwCaps           = DDSCAPS_TEXTURE|DDSCAPS_COMPLEX|DDSCAPS_MIPMAP;
        ddsd2.ddpfPixelFormat.dwFlags  = DDPF_FOURCC;
        ddsd2.ddpfPixelFormat.dwFlags |= DDPF_ALPHAPIXELS;

        switch (texture.format)
        {
            case CMP_FORMAT_BC1:
                ddsd2.ddpfPixelFormat.dwFourCC = FOURCC_DXT1;
                break;
            case CMP_FORMAT_BC2:
                ddsd2.ddpfPixelFormat.dwFourCC = FOURCC_DXT3;
                break;
            case CMP_FORMAT_BC3:
                ddsd2.ddpfPixelFormat.dwFourCC = FOURCC_DXT5;
                break;
            case CMP_FORMAT_BC4:
                ddsd2.ddpfPixelFormat.dwFourCC = FOURCC_ATI1N;
                break;
            case CMP_FORMAT_BC5:
                ddsd2.ddpfPixelFormat.dwFourCC = FOURCC_ATI2N_XY;
              break;
        }

        ddsd2.ddpfPixelFormat.dwPrivateFormatBitCount = 0;

        // Write the data
        fwrite(&ddsd2, sizeof(DDSD2), 1, pFile);
        fwrite(texture.pData, texture.dwDataSize, 1, pFile);
    }
    break;
    }

    if (!DDSet) {
        fwrite(&DDS_HEADER, sizeof(DWORD), 1, pFile);

        DDSD2 ddsd;
        memset(&ddsd, 0, sizeof(DDSD2));

        ddsd.dwSize = sizeof(DDSD2);
        ddsd.dwFlags = DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT|DDSD_MIPMAPCOUNT|DDSD_LINEARSIZE;
        ddsd.dwWidth = texture.dwWidth;
        ddsd.dwHeight = texture.dwHeight;
        ddsd.dwMipMapCount = 1;

        ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
        ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE|DDSCAPS_COMPLEX|DDSCAPS_MIPMAP;

        // Do we have a DX9 support FourCC format
        ddsd.ddpfPixelFormat.dwFourCC = GetFourCC(texture.format);
        if(ddsd.ddpfPixelFormat.dwFourCC) {
            ddsd.dwLinearSize = texture.dwDataSize;
            ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC;

            // Do we have DXT5 swizzle format
            if(IsDXT5SwizzledFormat(texture.format)) {
                ddsd.ddpfPixelFormat.dwPrivateFormatBitCount = ddsd.ddpfPixelFormat.dwFourCC;
                ddsd.ddpfPixelFormat.dwFourCC = FOURCC_DXT5;
            }

            fwrite(&ddsd, sizeof(DDSD2), 1, pFile);
            fwrite(texture.pData, texture.dwDataSize, 1, pFile);
        } else {
            // Check to save the data using DX10 file format (BC7 is used as an example of what is supported
            // and can be expanded to include other formats

            if ((texture.format == CMP_FORMAT_BC7)  ||
                    (texture.format == CMP_FORMAT_BC6H) ||
                    (texture.format == CMP_FORMAT_BC6H_SF) ) {
                ddsd.ddpfPixelFormat.dwFlags  = DDPF_FOURCC;
                ddsd.ddpfPixelFormat.dwFourCC = MAKEFOURCC('D', 'X', '1', '0');
                ddsd.lPitch = texture.dwWidth * 4;

                // Write the data
                fwrite(&ddsd, sizeof(DDSD2), 1, pFile);

                DDS_HEADER_DDS10 HeaderDDS10;
                memset(&HeaderDDS10, 0, sizeof(HeaderDDS10));

                if (texture.format == CMP_FORMAT_BC7)
                    HeaderDDS10.dxgiFormat = DXGI_FORMAT_BC7_UNORM;
                else if (texture.format == CMP_FORMAT_BC6H)
                    HeaderDDS10.dxgiFormat = DXGI_FORMAT_BC6H_UF16;
                else if (texture.format == CMP_FORMAT_BC6H_SF)
                    HeaderDDS10.dxgiFormat = DXGI_FORMAT_BC6H_SF16;

                HeaderDDS10.resourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
                HeaderDDS10.miscFlag = 0;
                HeaderDDS10.arraySize = 1;
                HeaderDDS10.reserved = 0;

                fwrite(&HeaderDDS10, sizeof(HeaderDDS10), 1, pFile);
                fwrite(texture.pData, texture.dwDataSize, 1, pFile);
            } else {
                //-------------------------------------
                // We can use DX9 file format to save
                //-------------------------------------
                switch (texture.format) {
                case CMP_FORMAT_RGBA_8888:
                    ddsd.ddpfPixelFormat.dwRBitMask         = 0xff000000;
                    ddsd.ddpfPixelFormat.dwGBitMask         = 0x00ff0000;
                    ddsd.ddpfPixelFormat.dwBBitMask         = 0x0000ff00;
                    ddsd.ddpfPixelFormat.dwRGBAlphaBitMask  = 0x000000ff;
                    ddsd.lPitch = texture.dwPitch;
                    ddsd.ddpfPixelFormat.dwRGBBitCount = 32;
                    ddsd.ddpfPixelFormat.dwFlags = DDPF_ALPHAPIXELS | DDPF_RGB;
                    break;
                case CMP_FORMAT_ARGB_8888:
                    ddsd.ddpfPixelFormat.dwRGBAlphaBitMask  = 0xff000000;
                    ddsd.ddpfPixelFormat.dwRBitMask         = 0x00ff0000;
                    ddsd.ddpfPixelFormat.dwGBitMask         = 0x0000ff00;
                    ddsd.ddpfPixelFormat.dwBBitMask         = 0x000000ff;
                    ddsd.lPitch = texture.dwPitch;
                    ddsd.ddpfPixelFormat.dwRGBBitCount = 32;
                    ddsd.ddpfPixelFormat.dwFlags = DDPF_ALPHAPIXELS | DDPF_RGB;
                    break;

                case CMP_FORMAT_RGB_888:
                    ddsd.ddpfPixelFormat.dwRBitMask = 0x00ff0000;
                    ddsd.ddpfPixelFormat.dwGBitMask = 0x0000ff00;
                    ddsd.ddpfPixelFormat.dwBBitMask = 0x000000ff;
                    ddsd.lPitch = texture.dwPitch;
                    ddsd.ddpfPixelFormat.dwRGBBitCount = 24;
                    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
                    break;

                case CMP_FORMAT_RG_8:
                    ddsd.ddpfPixelFormat.dwRBitMask = 0x0000ff00;
                    ddsd.ddpfPixelFormat.dwGBitMask = 0x000000ff;
                    ddsd.lPitch = texture.dwPitch;
                    ddsd.ddpfPixelFormat.dwRGBBitCount = 16;
                    ddsd.ddpfPixelFormat.dwFlags = DDPF_ALPHAPIXELS | DDPF_LUMINANCE;
                    ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0xff000000;
                    break;

                case CMP_FORMAT_R_8:
                    ddsd.ddpfPixelFormat.dwRBitMask = 0x000000ff;
                    ddsd.lPitch = texture.dwPitch;
                    ddsd.ddpfPixelFormat.dwRGBBitCount = 8;
                    ddsd.ddpfPixelFormat.dwFlags = DDPF_LUMINANCE;
                    break;

                case CMP_FORMAT_ARGB_2101010:
                    ddsd.ddpfPixelFormat.dwRBitMask = 0x000003ff;
                    ddsd.ddpfPixelFormat.dwGBitMask = 0x000ffc00;
                    ddsd.ddpfPixelFormat.dwBBitMask = 0x3ff00000;
                    ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0xc0000000;
                    ddsd.lPitch = texture.dwPitch;
                    ddsd.ddpfPixelFormat.dwRGBBitCount = 32;
                    ddsd.ddpfPixelFormat.dwFlags = DDPF_ALPHAPIXELS | DDPF_RGB;
                    break;

                case CMP_FORMAT_ARGB_16:
                    ddsd.dwLinearSize = texture.dwDataSize;
                    ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC | DDPF_ALPHAPIXELS;
                    ddsd.ddpfPixelFormat.dwFourCC = D3DFMT_A16B16G16R16;
                    break;

                case CMP_FORMAT_RG_16:
                    ddsd.dwLinearSize = texture.dwDataSize;
                    ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
                    ddsd.ddpfPixelFormat.dwFourCC = D3DFMT_G16R16;
                    break;

                case CMP_FORMAT_R_16:
                    ddsd.dwLinearSize = texture.dwDataSize;
                    ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
                    ddsd.ddpfPixelFormat.dwFourCC = D3DFMT_L16;
                    break;

                case CMP_FORMAT_ARGB_16F:
                    ddsd.dwLinearSize = texture.dwDataSize;
                    ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC | DDPF_ALPHAPIXELS;
                    ddsd.ddpfPixelFormat.dwFourCC = D3DFMT_A16B16G16R16F;
                    break;

                case CMP_FORMAT_RG_16F:
                    ddsd.dwLinearSize = texture.dwDataSize;
                    ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
                    ddsd.ddpfPixelFormat.dwFourCC = D3DFMT_G16R16F;
                    break;

                case CMP_FORMAT_R_16F:
                    ddsd.dwLinearSize = texture.dwDataSize;
                    ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
                    ddsd.ddpfPixelFormat.dwFourCC = D3DFMT_R16F;
                    break;

                case CMP_FORMAT_ARGB_32F:
                    ddsd.dwLinearSize = texture.dwDataSize;
                    ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC | DDPF_ALPHAPIXELS;
                    ddsd.ddpfPixelFormat.dwFourCC = D3DFMT_A32B32G32R32F;
                    break;

                case CMP_FORMAT_RG_32F:
                    ddsd.dwLinearSize = texture.dwDataSize;
                    ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
                    ddsd.ddpfPixelFormat.dwFourCC = D3DFMT_G32R32F;
                    break;

                case CMP_FORMAT_R_32F:
                    ddsd.dwLinearSize = texture.dwDataSize;
                    ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
                    ddsd.ddpfPixelFormat.dwFourCC = D3DFMT_R32F;
                    break;

                default:
                    assert(0);
                    break;
                }

                fwrite(&ddsd, sizeof(DDSD2), 1, pFile);
                fwrite(texture.pData, texture.dwDataSize, 1, pFile);
            }
        }
    } // DDSet

    fclose(pFile);
}
