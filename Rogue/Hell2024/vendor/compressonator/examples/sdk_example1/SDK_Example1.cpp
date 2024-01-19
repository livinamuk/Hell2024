// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved
// Copyright (c) 2004-2006    ATI Technologies Inc.
//
// Example1: Console application that demonstrates how to use the Compressonator SDK Lib
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
// Example1: Console application that demonstrates using SDK API CMP_ConvertTexture()
//
// SDK files required for application:
//     Compressontor.h
//     Compressonator_xx.lib  For static libs xx is either MD, MT or MDd or MTd, 
//                      When using DLL builds make sure the  Compressonator_xx_DLL.dll is in exe path

#include "Compressonator.h"

// The Helper code is provided to load Textures from a DDS file (Support Images and Compressed formats supported by DX9)
// Compressed images can also be saved using DDS File (Support Images and Compression format supported by DX9 and partial
// formats for DX10)
//
// A newer DDS loader utility is provided in V3.2 SDK : CMP_LoadTexture and CMP_SaveTexture, These use the MipMap structure
// to hold the image buffers used for processing.
//

#include "DDS_Helpers.h"

#include <stdio.h>
#include <string>

// Example code using high level SDK API's optimized with MultiThreading for processing a single image
// Note: No error checking is done on user arguments, so all parameters must be correct in this example
// (files must exist, values correct format, etc..)

#ifdef _WIN32
#include <windows.h>
#include <time.h>
double timeStampsec() {
    static LARGE_INTEGER frequency;
    if (frequency.QuadPart == 0)
        QueryPerformanceFrequency(&frequency);

    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return now.QuadPart / double(frequency.QuadPart);
}
#endif

bool g_bAbortCompression = false;   // If set true current compression will abort

//---------------------------------------------------------------------------
// Sample loop back code called for each compression block been processed
//---------------------------------------------------------------------------
bool CompressionCallback(float fProgress, CMP_DWORD_PTR pUser1, CMP_DWORD_PTR pUser2) {
    std::printf("\rCompression progress = %2.0f  ", fProgress);
    return g_bAbortCompression;
}

int main(int argc, const char* argv[]) {
#ifdef _WIN32
    double start_time = timeStampsec();
#endif
    if (argc < 5) {
        std::printf("Example1 SourceFile DestFile Format Quality\n");
        std::printf("This example shows how to compress a single image\n");
        std::printf("to a compression format using a quality setting\n");
        std::printf("Quality is in the range of 0.0 to 1.0\n");
        std::printf("When using DLL builds make sure the Compressonator_xx_DLL.dll is in exe path\n");
        std::printf("Usage: Example1.exe ruby.dds ruby_bc7.dds BC7 0.05\n");
        std::printf("this will generate a compressed ruby file in BC7 format\n");
        return 0;
    }

    const char*     pszSourceFile = argv[1];
    const char*     pszDestFile   = argv[2];
    CMP_FORMAT      destFormat    = ParseFormat(argv[3]);
    CMP_FLOAT       fQuality;

    try {
      fQuality = std::stof(argv[4]);
      if (fQuality < 0.0f) {
        fQuality = 0.0f;
        std::printf("Warning: Quality setting is out of range using 0.0\n");
      }
      if (fQuality > 1.0f) {
        fQuality = 1.0f;
        std::printf("Warning: Quality setting is out of range using 1.0\n");
      }
    } catch (...) {
      std::printf("Error: Unable to process quality setting\n");
      return -1;
    }

    if (destFormat == CMP_FORMAT_Unknown) {
        std::printf("Unsupported destination format\n");
        return 0;
    }

    //==========================================
    // Load Source Texture
    //==========================================
    CMP_Texture srcTexture;
    if (!LoadDDSFile(pszSourceFile, srcTexture)) {
        std::printf("Error loading source file!\n");
        return 0;
    }

    //==========================================================================
    // Check Source Texture format:
    // This example only works on 32 bit per pixel buffers formated as RGBA:8888
    // if the source is < 32 bit exit, 
    //==========================================================================
    if (!((srcTexture.format == CMP_FORMAT_RGBA_8888) || 
          (srcTexture.format == CMP_FORMAT_BGRA_8888))) {
        std::printf("Error This example works only on 32 bit per pixel image sources!\n");
        return 0;
    }

    //==========================================================================
    // if the source format is BGRA swizzle it to RGBA_8888
    //==========================================================================
    if (srcTexture.format == CMP_FORMAT_BGRA_8888)
    {
        unsigned char blue;
        for (CMP_DWORD i = 0; i < srcTexture.dwDataSize; i += 4)
        {
            blue = srcTexture.pData[i];
            srcTexture.pData[i] = srcTexture.pData[i + 2];
            srcTexture.pData[i + 2] = blue;
        }
        srcTexture.format = CMP_FORMAT_RGBA_8888;
    }

    //===================================
    // Initialize Compressed Destination
    //===================================
    CMP_Texture destTexture = {0};
    destTexture.dwSize     = sizeof(destTexture);
    destTexture.dwWidth    = srcTexture.dwWidth;
    destTexture.dwHeight   = srcTexture.dwHeight;
    destTexture.dwPitch    = srcTexture.dwWidth;
    destTexture.format     = destFormat;
    destTexture.dwDataSize = CMP_CalculateBufferSize(&destTexture);
    destTexture.pData = (CMP_BYTE*)malloc(destTexture.dwDataSize);

    //==========================
    // Set Compression Options
    //==========================
    CMP_CompressOptions options = {0};
    options.dwSize       = sizeof(options);
    options.fquality     = fQuality;
    options.dwnumThreads = 0;  // Uses auto, else set number of threads from 1..127 max

    //==========================
    // Compress Texture
    //==========================
    CMP_ERROR   cmp_status;
    cmp_status = CMP_ConvertTexture(&srcTexture, &destTexture, &options, &CompressionCallback);
    if (cmp_status != CMP_OK) {
        free(srcTexture.pData);
        free(destTexture.pData);
        std::printf("Compression returned an error %d\n", cmp_status);
        return cmp_status;
    }

    //==========================
    // Save Compressed Testure
    //==========================
    SaveDDSFile(pszDestFile, destTexture);

    free(srcTexture.pData);
    free(destTexture.pData);

#ifdef _WIN32
    std::printf("\nProcessed in %.3f seconds\n", timeStampsec() - start_time);
#endif
    return 0;
}
