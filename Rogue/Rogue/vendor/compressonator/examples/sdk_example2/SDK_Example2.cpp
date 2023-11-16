// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved
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
// Example2: Console application that demonstrates how to use compression API in a
//           multithread environment
//
// SDK files required for application:
//     Compressontor.h
//     Compressonator_xx.lib  For static libs xx is either MD, MT or MDd or MTd, 
//                      When using DLL builds make sure the  Compressonator_xx_DLL.dll is in exe path

#include "Compressonator.h"

// The Helper code is provided to load Textures from a DDS file (Support Images and Compressed formats supported by DX9)
// Compressed images can also be saved using DDS File (Support Images and Compression format supported by DX9 and partial
// formats for DX10)

#include "DDS_Helpers.h"

#include <stdio.h>
#include <string>

// EXAMPLE2 is using high level SDK API's optimized with MultiThreading for processing multiple images

#define USE_EXAMPLE2

#if __cplusplus < 199711L
#error This library needs at least a C++11 compliant compiler
#endif


#include <thread>
#define MXT 2       // Max number of compressed sample formats to generate, this is limited by available system mem

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
    if (argc < 4) {
        std::printf("Example2.exe SourceFile BCnFormat1 BCnFormat2 Quality\n");
        std::printf("This example shows how to compress a single image into two\n");
        std::printf("different BCn compression formats using multi threading\n");
        std::printf("Quality is in the range of 0.0 to 1.0\n");
        std::printf("When using DLL builds make sure the Compressonator_xx_DLL.dll is in exe path\n");
        std::printf("Usage: Example2.exe ruby.dds BC1 BC7 0.05\n");
        std::printf("this will generate a result_0.dds for BC1\n");
        std::printf("and                  result_1.dds for BC7\n");
        return 0;
    }

    // Note: No error checking is done on user arguments, so all parameters must be correct in this example
    // (files must exist, values correct format, etc..)
    const char*     pszSourceFile       = argv[1];
    CMP_FORMAT      destFormat[MXT];
    CMP_FLOAT       fQuality;

    destFormat[0] = ParseFormat(argv[2]);
    destFormat[1] = ParseFormat(argv[3]);

    if ((destFormat[0] == CMP_FORMAT_Unknown) || (destFormat[1] == CMP_FORMAT_Unknown)) {
        std::printf("Error: Unsupported BCn destination format\n");
        return 0;
    }

    if (destFormat[0] == destFormat[1]) {
      std::printf("Error: Please try two different BCn formats for this example\n");
      return 0;
    }


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
    }
    catch (...) {
        std::printf("Error: Unable to process quality setting\n");
        return -1;
    }



    //==========================
    // Load Source Texture #1
    //==========================
    CMP_Texture srcTexture;
    if (!LoadDDSFile(pszSourceFile, srcTexture)) {
        std::printf("Error: Loading source file!\n");
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
    // Initialize Compressed Destinations
    //===================================
    CMP_Texture destTexture[MXT];

    for (int i = 0; i<MXT; i++) {
        destTexture[i].dwSize     = sizeof(destTexture[i]);
        destTexture[i].dwWidth    = srcTexture.dwWidth;
        destTexture[i].dwHeight   = srcTexture.dwHeight;
        destTexture[i].dwPitch    = 0;
        destTexture[i].format     = destFormat[i];
        destTexture[i].dwDataSize = CMP_CalculateBufferSize(&destTexture[i]);
        destTexture[i].pData = (CMP_BYTE*)malloc(destTexture[i].dwDataSize);
    }

    //=======================================
    // Set Compression Options for Textures
    //=======================================
    CMP_CompressOptions options = {0};
    options.dwSize       = sizeof(options);
    options.fquality     = fQuality;            // Quality
    options.dwnumThreads = 0;                   // Number of threads to use per texture set to auto

    //=====================================================
    // Compress the Texture to multiple compressed formats
    //=====================================================
    CMP_ERROR cmp_status;
    try {
        //--------------------------------------------------------------------------------
        // Issue note: cmp_status3 is not used as an array!.
        // ie cmp_status3[MXT] in lambda calls - so status of results is Un-deterministic!
        //--------------------------------------------------------------------------------
        std::thread t3[MXT];
        for (int i =0; i<MXT; i++)
            t3[i] = std::thread([&]() {
            cmp_status = CMP_ConvertTexture(&srcTexture, &destTexture[i], &options, &CompressionCallback);
        });

        // Finish the encoders
        for (int i = 0; i<MXT; i++)
            t3[i].join();
    } catch (const std::exception& ex) {
        std::printf("Error: %s\n",ex.what());
    }

    //======================================
    // Save Compressed Textures To DDS Files
    //======================================
    std::string str;
    if (cmp_status == CMP_OK) {
        for (int i = 0; i < MXT; i++) {
            str.clear();
            str.append("result_");
            str.append(std::to_string(i).c_str());
            str.append(".dds");
            SaveDDSFile(str.c_str(), destTexture[i]);
        }
    }

    if (srcTexture.pData)  free(srcTexture.pData);
    for (int i = 0; i < MXT; i++) {
        if (destTexture[i].pData) free(destTexture[i].pData);
    }

    std::printf("\n");
#ifdef _WIN32
    std::printf("\nProcessed in %.3f seconds\n", timeStampsec() - start_time);
#endif

    return 0;
}
