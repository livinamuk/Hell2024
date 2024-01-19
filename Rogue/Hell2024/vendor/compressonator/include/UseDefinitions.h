//==============================================================================
// Copyright (c) 2007-2020    Advanced Micro Devices, Inc. All rights reserved.
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
/// \file UseDefinitions.h  Common Feature Enablement in Compressonator SDK
//
//==============================================================================

// External Libs
#ifndef USE_BOOST
 #define USE_BOOST                                     //  Enable using Boost code
#endif

#ifndef USE_ASSIMP
// #define USE_ASSIMP
#endif

#ifndef USE_CONVECTION_KERNELS
// #define USE_CONVECTION_KERNELS                     // Test External Lib : ConvectionKernels for quality& performance, to enable use q=0.1 in HPC
#endif

#ifndef USE_GPU_PIPELINE_VULKAN
//#define USE_GPU_PIPELINE_VULKAN                     // Enable to use CMP_Core codecs on a Vulkan Pipeline
#endif

#ifndef USE_CPU_PERFORMANCE_COUNTERS
//#define USE_CPU_PERFORMANCE_COUNTERS               // Enable to use performance monitoring with CPU high precision counters in place of GPU query performance counters
#endif

//#define TEST_CMP_CORE_DECODER                       //  Test CMP_Core Decoders using Compressonator SDK Decoders, 
#define USE_QTWEBKIT                                  //  Enable Qt Webengine interfaces for GUI welcome page
//#define BC7_DEBUG_TO_RESULTS_TXT                    //  Send debug info to a results text file
//#define DXT5_COMPDEBUGGER                           //  Remote connect data to Comp Debugger views
//#define BC6H_COMPDEBUGGER                           //  Remote connect data to Comp Debugger views
//#define BC7_COMPDEBUGGER                            //  Remote connect data to Comp Debugger views
//#define BC6H_NO_OPTIMIZE_ENDPOINTS                  //  Turn off BC6H optimization of endpoints - useful for debugging quantization and mode checking
//#define BC6H_DEBUG_TO_RESULTS_TXT                   //  Generates a Results.txt file on exe working directory; MultiThreading is turned off for debuging to text file
//#define BC6H_DECODE_DEBUG                           //  Enables debug info on decoder
//#define GT_COMPDEBUGGER                             //  Remote connect data to Comp Debugger views

#ifndef ENABLE_MAKE_COMPATIBLE_API  
#define ENABLE_MAKE_COMPATIBLE_API  //  Byte<->Float to make all source and dest compatible
#endif

// V2.4 / V2.5 features and changes
#define USE_OLD_SWIZZLE                             //  Remove swizzle flag and abide by CMP_Formats

// Model changes
#define USE_MESH_CLI                                // CLI Process Mesh (only support glTF and OBJ files)

#ifndef CMAKE_DRACO_NOT_FOUND
#define USE_MESH_DRACO_EXTENSION                    // Mesh Compression with Draco support in glTF and OBJ files only
#endif

// todo: multiple mesh decompression is still under development. Enable define below will generate corrupted view.
// #define USE_MULTIPLE_MESH_DECODE                 // Enable multiple meshes and multiple primitives draco decompression

// Codec options
#define USE_ETCPACK                                 // Use ETCPack for ETC2 else use CModel code!

// todo: recommended to use default setting for now as the settings for different draco level may produce corrupted textures.
// #define USE_MESH_DRACO_SETTING                   // Expose draco settings for draco mesh compression, 
                                                    // if disabled default setting will be used for mesh compression

// #define USE_GLTF2_MIPSET                         // Enable Image Transcode & Compression support for GLTF files using TextureIO
// #define USE_FILEIO                               // Used for debugging code

// New features under devlopement
// #define USE_GTC                                  //  LDR Gradient Texture Compressor patent pending
// #define USE_BASIS                                //  Future release:: Universal format for transcoding codecs
// #define USE_CMP_TRANSCODE                        //  Future release:: Transcode BASIS/GTC to other compressed formats

// To Be enabled in future releases
// #define ARGB_32_SUPPORT                           // Enables 32bit Float channel formats
// #define SUPPORT_ETC_ALPHA                         // for ATC_RGB output enable A
// #define SHOW_PROCESS_MEMORY                       // display available CPU process memory
// #define USE_BCN_IMAGE_DEBUG                       // Enables Combobox in Image View for low level BCn based block compression in debug mode
// #define USE_CRN                                   // Enabled .crn file output using CRUNCH encoder
// #define USE_3DCONVERT                             // Enable 3D model conversion (glTF<->obj) icon
// #define ENABLE_USER_ETC2S_FORMATS                 // Enable users to set these formats in CLI and GUI applications

