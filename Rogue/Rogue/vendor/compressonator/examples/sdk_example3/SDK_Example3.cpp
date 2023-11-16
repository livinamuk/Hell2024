// Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved
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
// Example3: Console application that demonstrates how to use the block level SDK API
//
// SDK files required for application:
//     Compressontor.h
//     Compressonator_xx.lib  For static libs xx is either MD, MT or MDd or MTd, 
//                      When using DLL builds make sure the  Compressonator_xx_DLL.dll is in exe path

#include <string>

#include "Compressonator.h"

// The Helper code is provided to load Textures from a DDS file (Support Images and Compressed formats supported by DX9)
// Compressed images can also be saved using DDS File (Support Images and Compression format supported by DX9 and partial
// formats for DX10)

#include "DDS_Helpers.h"

// Example of low level API that give access to compression blocks (4x4) for BC7

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
    UNREFERENCED_PARAMETER(pUser1);
    UNREFERENCED_PARAMETER(pUser2);

    std::printf("\rCompression progress = %2.0f  ", fProgress);

    return g_bAbortCompression;
}

int main(int argc, const char* argv[]) {
#ifdef _WIN32
    double start_time = timeStampsec();
#endif
    if (argc < 4) {
        std::printf("Example3 SourceFile DestFile Quality\n");
        std::printf("This example shows how to compress a single image\n");
        std::printf("to a BC7 compression format using single threaded low level\n");
        std::printf("compression blocks access with a quality setting\n");
        std::printf("Quality is in the range of 0.0 to 1.0\n");
        std::printf("** NOTE ** sample image Width and Height must be divisable by 4\n\n");
        std::printf("When using DLL builds make sure the Compressonator_xx_DLL.dll is in exe path\n");
        std::printf("Usage: Example3.exe sample.dds sample_bc7.dds 0.05\n");
        std::printf("this will generate a compressed file in BC7 format\n");
        return 0;
    }

    // Note: No error checking is done on user arguments, so all parameters must be correct in this example
    // (files must exist, values correct format, etc..)
    const char*     pszSourceFile = argv[1];
    const char*     pszDestFile   = argv[2];
    CMP_FORMAT      destFormat    = CMP_FORMAT_BC7;
    CMP_FLOAT       fQuality;

    try {
      fQuality = std::stof(argv[3]);
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

    // Load the source texture
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
      std::printf(
          "Error This example works only on 32 bit per pixel image sources!\n");
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


    // Init dest memory to use for compressed texture
    CMP_Texture destTexture;
    destTexture.dwSize     = sizeof(destTexture);
    destTexture.dwWidth    = srcTexture.dwWidth;
    destTexture.dwHeight   = srcTexture.dwHeight;
    destTexture.dwPitch    = 0;
    destTexture.format     = destFormat;
    destTexture.dwDataSize = CMP_CalculateBufferSize(&destTexture);
    destTexture.pData = (CMP_BYTE*)malloc(destTexture.dwDataSize);

    BC_ERROR   cmp_status;

    // Example 2 : Using Low level block access code valid only BC7 (and BC6H not shown in this example)
    if (destTexture.format == CMP_FORMAT_BC7) {

        // Step 1: Initialize the Codec: Need to call it only once, repeated calls will return BC_ERROR_LIBRARY_ALREADY_INITIALIZED
        if (CMP_InitializeBCLibrary() != BC_ERROR_NONE) {
            std::printf("BC Codec already initialized!\n");
        }

        // Step 2: Create a BC7 Encoder
        BC7BlockEncoder *BC7Encoder;

        // Note we are setting quality low for faster encoding in this sample
        CMP_CreateBC7Encoder(
            0.05,               // Quality set to low
            0,                  // Do not restrict colors
            0,                  // Do not restrict alpha
            0xFF,               // Use all BC7 modes
            1,                  // Performance set to optimal
            &BC7Encoder);

        // Pointer to source data
        CMP_BYTE *pdata = (CMP_BYTE *)srcTexture.pData;

        const CMP_DWORD dwBlocksX = ((srcTexture.dwWidth  + 3) >> 2);
        const CMP_DWORD dwBlocksY = ((srcTexture.dwHeight + 3) >> 2);
        const CMP_DWORD dwBlocksXY = dwBlocksX*dwBlocksY;

        CMP_DWORD dstIndex  = 0;    // Destination block index
        CMP_DWORD srcStride = srcTexture.dwWidth * 4;

        // Step 4: Process the blocks
        for (CMP_DWORD yBlock = 0; yBlock < dwBlocksY; yBlock++) {

            for (CMP_DWORD xBlock = 0; xBlock < dwBlocksX; xBlock++) {

              // Source block index start base: top left pixel of the 4x4 block
              CMP_DWORD srcBlockIndex = (yBlock * srcStride * 4) + xBlock*16;

                // Get a input block of data to encode
                // Currently the BC7 encoder is using double data formats
                double blockToEncode[16][4];
                CMP_DWORD srcIndex;
                for (int row = 0; row < 4; row++) {
                    srcIndex = srcBlockIndex + (srcStride * row);
                    for (int col = 0; col < 4; col++) {
                        blockToEncode[row*4 + col][BC_COMP_RED]   = (double)*(pdata+srcIndex++);
                        blockToEncode[row*4 + col][BC_COMP_GREEN] = (double)*(pdata+srcIndex++);
                        blockToEncode[row*4 + col][BC_COMP_BLUE]  = (double)*(pdata+srcIndex++);
                        blockToEncode[row*4 + col][BC_COMP_ALPHA] = (double)*(pdata+srcIndex++);
                    }
                }

                // Call the block encoder : output is 128 bit compressed data
                cmp_status = CMP_EncodeBC7Block(BC7Encoder, blockToEncode, (destTexture.pData + dstIndex));
                if (cmp_status != BC_ERROR_NONE) {
                  std::printf(
                      "Compression error at block X = %d Block Y = %d \n",xBlock, yBlock);
                }
                dstIndex += 16;

                // Show Progress
                float fProgress = 100.f * (yBlock * dwBlocksX) / dwBlocksXY;

                std::printf("\rCompression progress = %2.0f", fProgress);

            }
        }

        // Step 5 Free up the BC7 Encoder
        CMP_DestroyBC7Encoder(BC7Encoder);

        // Step 6 Close the BC Codec
        CMP_ShutdownBCLibrary();
    }

    // Save the results
    if (cmp_status == CMP_OK)
        SaveDDSFile(pszDestFile, destTexture);

    // Clean up memory used for textures
    free(srcTexture.pData);
    free(destTexture.pData);

#ifdef _WIN32
    std::printf("\nProcessed in %.3f seconds\n", timeStampsec() - start_time);
#endif
    return 0;
}
