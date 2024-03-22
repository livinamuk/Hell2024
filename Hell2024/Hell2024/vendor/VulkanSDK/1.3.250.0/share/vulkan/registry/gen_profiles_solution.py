#!/usr/bin/python3
#
# Copyright (c) 2021-2023 LunarG, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License")
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Author: Daniel Rakos, RasterGrid

import os
import re
import itertools
import functools
import argparse
from typing import OrderedDict
import xml.etree.ElementTree as etree
import json
import jsonschema
from collections import deque

def apiNameMatch(str, supported):
    """Return whether a required api name matches a pattern specified for an
    XML <feature> 'api' attribute or <extension> 'supported' attribute.
    - str - API name such as 'vulkan' or 'openxr'. May be None, in which
        case it never matches (this should not happen).
    - supported - comma-separated list of XML API names. May be None, in
        which case str always matches (this is the usual case)."""

    if str is not None:
        return supported is None or str in supported.split(',')

    # Fallthrough case - either str is None or the test failed
    return False

def stripNonmatchingAPIs(tree, apiName, actuallyDelete = True):
    """Remove tree Elements with 'api' attributes matching apiName.
        tree - Element at the root of the hierarchy to strip. Only its
            children can actually be removed, not the tree itself.
        apiName - string which much match a command-separated component of
            the 'api' attribute.
        actuallyDelete - only delete matching elements if True."""

    stack = deque()
    stack.append(tree)

    while len(stack) > 0:
        parent = stack.pop()

        for child in parent.findall('*'):
            api = child.get('api')

            if apiNameMatch(apiName, api):
                # Add child to the queue
                stack.append(child)
            elif not apiNameMatch(apiName, api):
                # Child does not match requested api. Remove it.
                if actuallyDelete:
                    parent.remove(child)

COPYRIGHT_HEADER = '''/**
 * Copyright (c) 2021-2023 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License")
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * DO NOT EDIT: This file is generated.
 */
'''

DEBUG_MSG_CB_DEFINE = '''
#include <stdio.h>

#ifndef VP_DEBUG_MESSAGE_CALLBACK
#if defined(ANDROID) || defined(__ANDROID__)
#include <android/log.h>
#define VP_DEBUG_MESSAGE_CALLBACK(MSG) \
    __android_log_print(ANDROID_LOG_ERROR, "Profiles ERROR", "%s", MSG); \\
    __android_log_print(ANDROID_LOG_DEBUG, "Profiles WARNING", "%s", MSG)
#else
#define VP_DEBUG_MESSAGE_CALLBACK(MSG) fprintf(stderr, "%s\\n", MSG)
#endif
#else
void VP_DEBUG_MESSAGE_CALLBACK(const char*);
#endif

#define VP_DEBUG_MSG(MSG) VP_DEBUG_MESSAGE_CALLBACK(MSG)
#define VP_DEBUG_MSGF(MSGFMT, ...) { char msg[1024]; snprintf(msg, sizeof(msg) - 1, (MSGFMT), __VA_ARGS__); VP_DEBUG_MESSAGE_CALLBACK(msg); }
#define VP_DEBUG_COND_MSG(COND, MSG) if (COND) VP_DEBUG_MSG(MSG)
#define VP_DEBUG_COND_MSGF(COND, MSGFMT, ...) if (COND) VP_DEBUG_MSGF(MSGFMT, __VA_ARGS__)
'''

DEBUG_MSG_UTIL_IMPL = '''
#include <string>

namespace detail {

VPAPI_ATTR std::string vpGetDeviceAndDriverInfoString(VkPhysicalDevice physicalDevice,
                                                      PFN_vkGetPhysicalDeviceProperties2KHR pfnGetPhysicalDeviceProperties2) {
    VkPhysicalDeviceDriverPropertiesKHR driverProps{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES_KHR };
    VkPhysicalDeviceProperties2KHR deviceProps{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR, &driverProps };
    pfnGetPhysicalDeviceProperties2(physicalDevice, &deviceProps);
    return std::string("deviceName=") + std::string(&deviceProps.properties.deviceName[0])
                    + ", driverName=" + std::string(&driverProps.driverName[0])
                    + ", driverInfo=" + std::string(&driverProps.driverInfo[0]);
}

}
'''

H_HEADER = '''
#ifndef VULKAN_PROFILES_H_
#define VULKAN_PROFILES_H_ 1

#define VPAPI_ATTR

#ifdef __cplusplus
    extern "C" {
#endif

#include <vulkan/vulkan.h>
'''

H_FOOTER = '''
#ifdef __cplusplus
}
#endif

#endif // VULKAN_PROFILES_H_
'''

CPP_HEADER = '''
#include <cstddef>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <vector>
#include <algorithm>
'''

HPP_HEADER = '''
#ifndef VULKAN_PROFILES_HPP_
#define VULKAN_PROFILES_HPP_ 1

#define VPAPI_ATTR inline

#include <vulkan/vulkan.h>
#include <cstddef>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <vector>
#include <algorithm>
'''

HPP_FOOTER = '''
#endif // VULKAN_PROFILES_HPP_
'''

API_DEFS = '''
#define VP_MAX_PROFILE_NAME_SIZE 256U

typedef struct VpProfileProperties {
    char        profileName[VP_MAX_PROFILE_NAME_SIZE];
    uint32_t    specVersion;
} VpProfileProperties;

typedef enum VpInstanceCreateFlagBits {
    // Default behavior:
    // - profile extensions are used (application must not specify extensions)

    // Merge application provided extension list and profile extension list
    VP_INSTANCE_CREATE_MERGE_EXTENSIONS_BIT = 0x00000001,

    // Use application provided extension list
    VP_INSTANCE_CREATE_OVERRIDE_EXTENSIONS_BIT = 0x00000002,

    VP_INSTANCE_CREATE_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
} VpInstanceCreateFlagBits;
typedef VkFlags VpInstanceCreateFlags;

typedef struct VpInstanceCreateInfo {
    const VkInstanceCreateInfo* pCreateInfo;
    const VpProfileProperties*  pProfile;
    VpInstanceCreateFlags       flags;
} VpInstanceCreateInfo;

typedef enum VpDeviceCreateFlagBits {
    // Default behavior:
    // - profile extensions are used (application must not specify extensions)
    // - profile feature structures are used (application must not specify any of them) extended
    //   with any other application provided struct that isn't defined by the profile

    // Merge application provided extension list and profile extension list
    VP_DEVICE_CREATE_MERGE_EXTENSIONS_BIT = 0x00000001,

    // Use application provided extension list
    VP_DEVICE_CREATE_OVERRIDE_EXTENSIONS_BIT = 0x00000002,

    // Merge application provided versions of feature structures with the profile features
    // Currently unsupported, but is considered for future inclusion in which case the
    // default behavior could potentially be changed to merging as the currently defined
    // default behavior is forward-compatible with that
    // VP_DEVICE_CREATE_MERGE_FEATURES_BIT = 0x00000004,

    // Use application provided versions of feature structures but use the profile feature
    // structures when the application doesn't provide one (robust access disable flags are
    // ignored if the application overrides the corresponding feature structures)
    VP_DEVICE_CREATE_OVERRIDE_FEATURES_BIT = 0x00000008,

    // Only use application provided feature structures, don't add any profile specific
    // feature structures (robust access disable flags are ignored in this case and only the
    // application provided structures are used)
    VP_DEVICE_CREATE_OVERRIDE_ALL_FEATURES_BIT = 0x00000010,

    VP_DEVICE_CREATE_DISABLE_ROBUST_BUFFER_ACCESS_BIT = 0x00000020,
    VP_DEVICE_CREATE_DISABLE_ROBUST_IMAGE_ACCESS_BIT = 0x00000040,
    VP_DEVICE_CREATE_DISABLE_ROBUST_ACCESS =
        VP_DEVICE_CREATE_DISABLE_ROBUST_BUFFER_ACCESS_BIT | VP_DEVICE_CREATE_DISABLE_ROBUST_IMAGE_ACCESS_BIT,

    VP_DEVICE_CREATE_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
} VpDeviceCreateFlagBits;
typedef VkFlags VpDeviceCreateFlags;

typedef struct VpDeviceCreateInfo {
    const VkDeviceCreateInfo*   pCreateInfo;
    const VpProfileProperties*  pProfile;
    VpDeviceCreateFlags         flags;
} VpDeviceCreateInfo;

// Query the list of available profiles in the library
VPAPI_ATTR VkResult vpGetProfiles(uint32_t *pPropertyCount, VpProfileProperties *pProperties);

// List the recommended fallback profiles of a profile
VPAPI_ATTR VkResult vpGetProfileFallbacks(const VpProfileProperties *pProfile, uint32_t *pPropertyCount, VpProfileProperties *pProperties);

// Check whether a profile is supported at the instance level
VPAPI_ATTR VkResult vpGetInstanceProfileSupport(const char *pLayerName, const VpProfileProperties *pProfile, VkBool32 *pSupported);

// Create a VkInstance with the profile instance extensions enabled
VPAPI_ATTR VkResult vpCreateInstance(const VpInstanceCreateInfo *pCreateInfo,
                                     const VkAllocationCallbacks *pAllocator, VkInstance *pInstance);

// Check whether a profile is supported by the physical device
VPAPI_ATTR VkResult vpGetPhysicalDeviceProfileSupport(VkInstance instance, VkPhysicalDevice physicalDevice,
                                                      const VpProfileProperties *pProfile, VkBool32 *pSupported);

// Create a VkDevice with the profile features and device extensions enabled
VPAPI_ATTR VkResult vpCreateDevice(VkPhysicalDevice physicalDevice, const VpDeviceCreateInfo *pCreateInfo,
                                   const VkAllocationCallbacks *pAllocator, VkDevice *pDevice);

// Query the list of instance extensions of a profile
VPAPI_ATTR VkResult vpGetProfileInstanceExtensionProperties(const VpProfileProperties *pProfile, uint32_t *pPropertyCount,
                                                            VkExtensionProperties *pProperties);

// Query the list of device extensions of a profile
VPAPI_ATTR VkResult vpGetProfileDeviceExtensionProperties(const VpProfileProperties *pProfile, uint32_t *pPropertyCount,
                                                          VkExtensionProperties *pProperties);

// Fill the feature structures with the requirements of a profile
VPAPI_ATTR void vpGetProfileFeatures(const VpProfileProperties *pProfile, void *pNext);

// Query the list of feature structure types specified by the profile
VPAPI_ATTR VkResult vpGetProfileFeatureStructureTypes(const VpProfileProperties *pProfile, uint32_t *pStructureTypeCount,
                                                      VkStructureType *pStructureTypes);

// Fill the property structures with the requirements of a profile
VPAPI_ATTR void vpGetProfileProperties(const VpProfileProperties *pProfile, void *pNext);

// Query the list of property structure types specified by the profile
VPAPI_ATTR VkResult vpGetProfilePropertyStructureTypes(const VpProfileProperties *pProfile, uint32_t *pStructureTypeCount,
                                                       VkStructureType *pStructureTypes);

// Query the requirements of queue families by a profile
VPAPI_ATTR VkResult vpGetProfileQueueFamilyProperties(const VpProfileProperties *pProfile, uint32_t *pPropertyCount,
                                                      VkQueueFamilyProperties2KHR *pProperties);

// Query the list of query family structure types specified by the profile
VPAPI_ATTR VkResult vpGetProfileQueueFamilyStructureTypes(const VpProfileProperties *pProfile, uint32_t *pStructureTypeCount,
                                                          VkStructureType *pStructureTypes);

// Query the list of formats with specified requirements by a profile
VPAPI_ATTR VkResult vpGetProfileFormats(const VpProfileProperties *pProfile, uint32_t *pFormatCount, VkFormat *pFormats);

// Query the requirements of a format for a profile
VPAPI_ATTR void vpGetProfileFormatProperties(const VpProfileProperties *pProfile, VkFormat format, void *pNext);

// Query the list of format structure types specified by the profile
VPAPI_ATTR VkResult vpGetProfileFormatStructureTypes(const VpProfileProperties *pProfile, uint32_t *pStructureTypeCount,
                                                     VkStructureType *pStructureTypes);
'''

PRIVATE_DEFS = '''
VPAPI_ATTR bool isMultiple(double source, double multiple) {
    double mod = std::fmod(source, multiple);
    return std::abs(mod) < 0.0001; 
}

VPAPI_ATTR bool isPowerOfTwo(double source) {
    double mod = std::fmod(source, 1.0);
    if (std::abs(mod) >= 0.0001) return false;

    std::uint64_t value = static_cast<std::uint64_t>(std::abs(source));
    return !(value & (value - static_cast<std::uint64_t>(1)));
}

using PFN_vpStructFiller = void(*)(VkBaseOutStructure* p);
using PFN_vpStructComparator = bool(*)(VkBaseOutStructure* p);
using PFN_vpStructChainerCb =  void(*)(VkBaseOutStructure* p, void* pUser);
using PFN_vpStructChainer = void(*)(VkBaseOutStructure* p, void* pUser, PFN_vpStructChainerCb pfnCb);

struct VpFeatureDesc {
    PFN_vpStructFiller              pfnFiller;
    PFN_vpStructComparator          pfnComparator;
    PFN_vpStructChainer             pfnChainer;
};

struct VpPropertyDesc {
    PFN_vpStructFiller              pfnFiller;
    PFN_vpStructComparator          pfnComparator;
    PFN_vpStructChainer             pfnChainer;
};

struct VpQueueFamilyDesc {
    PFN_vpStructFiller              pfnFiller;
    PFN_vpStructComparator          pfnComparator;
};

struct VpFormatDesc {
    VkFormat                        format;
    PFN_vpStructFiller              pfnFiller;
    PFN_vpStructComparator          pfnComparator;
};

struct VpStructChainerDesc {
    PFN_vpStructChainer             pfnFeature;
    PFN_vpStructChainer             pfnProperty;
    PFN_vpStructChainer             pfnQueueFamily;
    PFN_vpStructChainer             pfnFormat;
};

struct VpProfileDesc {
    VpProfileProperties             props;
    uint32_t                        minApiVersion;

    const VkExtensionProperties*    pInstanceExtensions;
    uint32_t                        instanceExtensionCount;

    const VkExtensionProperties*    pDeviceExtensions;
    uint32_t                        deviceExtensionCount;

    const VpProfileProperties*      pFallbacks;
    uint32_t                        fallbackCount;

    const VkStructureType*          pFeatureStructTypes;
    uint32_t                        featureStructTypeCount;
    VpFeatureDesc                   feature;

    const VkStructureType*          pPropertyStructTypes;
    uint32_t                        propertyStructTypeCount;
    VpPropertyDesc                  property;

    const VkStructureType*          pQueueFamilyStructTypes;
    uint32_t                        queueFamilyStructTypeCount;
    const VpQueueFamilyDesc*        pQueueFamilies;
    uint32_t                        queueFamilyCount;

    const VkStructureType*          pFormatStructTypes;
    uint32_t                        formatStructTypeCount;
    const VpFormatDesc*             pFormats;
    uint32_t                        formatCount;

    VpStructChainerDesc             chainers;
};

template <typename T>
VPAPI_ATTR bool vpCheckFlags(const T& actual, const uint64_t expected) {
    return (actual & expected) == expected;
}
'''

PRIVATE_IMPL_BODY = '''
VPAPI_ATTR const VpProfileDesc* vpGetProfileDesc(const char profileName[VP_MAX_PROFILE_NAME_SIZE]) {
    for (uint32_t i = 0; i < vpProfileCount; ++i) {
        if (strncmp(vpProfiles[i].props.profileName, profileName, VP_MAX_PROFILE_NAME_SIZE) == 0) return &vpProfiles[i];
    }
    return nullptr;
}

VPAPI_ATTR bool vpCheckVersion(uint32_t actual, uint32_t expected) {
    uint32_t actualMajor = VK_API_VERSION_MAJOR(actual);
    uint32_t actualMinor = VK_API_VERSION_MINOR(actual);
    uint32_t expectedMajor = VK_API_VERSION_MAJOR(expected);
    uint32_t expectedMinor = VK_API_VERSION_MINOR(expected);
    return actualMajor > expectedMajor || (actualMajor == expectedMajor && actualMinor >= expectedMinor);
}

VPAPI_ATTR bool vpCheckExtension(const VkExtensionProperties *supportedProperties, size_t supportedSize,
                                 const char *requestedExtension) {
    bool found = false;
    for (size_t i = 0, n = supportedSize; i < n; ++i) {
        if (strcmp(supportedProperties[i].extensionName, requestedExtension) == 0) {
            found = true;
            // Drivers don't actually update their spec version, so we cannot rely on this
            // if (supportedProperties[i].specVersion >= expectedVersion) found = true;
        }
    }
    VP_DEBUG_COND_MSGF(!found, "Unsupported extension: %s", requestedExtension);
    return found;
}

VPAPI_ATTR void vpGetExtensions(uint32_t requestedExtensionCount, const char *const *ppRequestedExtensionNames,
                                uint32_t profileExtensionCount, const VkExtensionProperties *pProfileExtensionProperties,
                                std::vector<const char *> &extensions, bool merge, bool override) {
    if (override) {
        for (uint32_t i = 0; i < requestedExtensionCount; ++i) {
            extensions.push_back(ppRequestedExtensionNames[i]);
        }
    } else {
        for (uint32_t i = 0; i < profileExtensionCount; ++i) {
            extensions.push_back(pProfileExtensionProperties[i].extensionName);
        }

        if (merge) {
            for (uint32_t i = 0; i < requestedExtensionCount; ++i) {
                if (vpCheckExtension(pProfileExtensionProperties, profileExtensionCount, ppRequestedExtensionNames[i])) {
                    continue;
                }
                extensions.push_back(ppRequestedExtensionNames[i]);
            }
        }
    }
}

VPAPI_ATTR const void* vpGetStructure(const void* pNext, VkStructureType type) {
    const VkBaseOutStructure *p = static_cast<const VkBaseOutStructure*>(pNext);
    while (p != nullptr) {
        if (p->sType == type) return p;
        p = p->pNext;
    }
    return nullptr;
}

VPAPI_ATTR void* vpGetStructure(void* pNext, VkStructureType type) {
    VkBaseOutStructure *p = static_cast<VkBaseOutStructure*>(pNext);
    while (p != nullptr) {
        if (p->sType == type) return p;
        p = p->pNext;
    }
    return nullptr;
}
'''

PUBLIC_IMPL_BODY = '''
VPAPI_ATTR VkResult vpGetProfiles(uint32_t *pPropertyCount, VpProfileProperties *pProperties) {
    VkResult result = VK_SUCCESS;

    if (pProperties == nullptr) {
        *pPropertyCount = detail::vpProfileCount;
    } else {
        if (*pPropertyCount < detail::vpProfileCount) {
            result = VK_INCOMPLETE;
        } else {
            *pPropertyCount = detail::vpProfileCount;
        }
        for (uint32_t i = 0; i < *pPropertyCount; ++i) {
            pProperties[i] = detail::vpProfiles[i].props;
        }
    }
    return result;
}

VPAPI_ATTR VkResult vpGetProfileFallbacks(const VpProfileProperties *pProfile, uint32_t *pPropertyCount, VpProfileProperties *pProperties) {
    VkResult result = VK_SUCCESS;

    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc == nullptr) return VK_ERROR_UNKNOWN;

    if (pProperties == nullptr) {
        *pPropertyCount = pDesc->fallbackCount;
    } else {
        if (*pPropertyCount < pDesc->fallbackCount) {
            result = VK_INCOMPLETE;
        } else {
            *pPropertyCount = pDesc->fallbackCount;
        }
        for (uint32_t i = 0; i < *pPropertyCount; ++i) {
            pProperties[i] = pDesc->pFallbacks[i];
        }
    }
    return result;
}

VPAPI_ATTR VkResult vpGetInstanceProfileSupport(const char *pLayerName, const VpProfileProperties *pProfile, VkBool32 *pSupported) {
    VkResult result = VK_SUCCESS;

    uint32_t apiVersion = VK_MAKE_VERSION(1, 0, 0);
    static PFN_vkEnumerateInstanceVersion pfnEnumerateInstanceVersion =
        (PFN_vkEnumerateInstanceVersion)vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion");
    if (pfnEnumerateInstanceVersion != nullptr) {
        result = pfnEnumerateInstanceVersion(&apiVersion);
        if (result != VK_SUCCESS) {
            return result;
        }
    }

    uint32_t extCount = 0;
    result = vkEnumerateInstanceExtensionProperties(pLayerName, &extCount, nullptr);
    if (result != VK_SUCCESS) {
        return result;
    }
    std::vector<VkExtensionProperties> ext(extCount);
    result = vkEnumerateInstanceExtensionProperties(pLayerName, &extCount, ext.data());
    if (result != VK_SUCCESS) {
        return result;
    }

    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc == nullptr) return VK_ERROR_UNKNOWN;

    *pSupported = VK_TRUE;

    if (pDesc->props.specVersion < pProfile->specVersion) {
        VP_DEBUG_MSGF("Unsupported profile version: %u", pProfile->specVersion);
        *pSupported = VK_FALSE;
    }

    if (!detail::vpCheckVersion(apiVersion, pDesc->minApiVersion)) {
        VP_DEBUG_MSGF("Unsupported API version: %u.%u.%u", VK_API_VERSION_MAJOR(pDesc->minApiVersion), VK_API_VERSION_MINOR(pDesc->minApiVersion), VK_API_VERSION_PATCH(pDesc->minApiVersion));
        *pSupported = VK_FALSE;
    }

    for (uint32_t i = 0; i < pDesc->instanceExtensionCount; ++i) {
        if (!detail::vpCheckExtension(ext.data(), ext.size(),
            pDesc->pInstanceExtensions[i].extensionName)) {
            *pSupported = VK_FALSE;
        }
    }

    // We require VK_KHR_get_physical_device_properties2 if we are on Vulkan 1.0
    if (apiVersion < VK_API_VERSION_1_1) {
        bool foundGPDP2 = false;
        for (size_t i = 0; i < ext.size(); ++i) {
            if (strcmp(ext[i].extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0) {
                foundGPDP2 = true;
                break;
            }
        }
        if (!foundGPDP2) {
            VP_DEBUG_MSG("Unsupported mandatory extension VK_KHR_get_physical_device_properties2 on Vulkan 1.0");
            *pSupported = VK_FALSE;
        }
    }

    return result;
}

VPAPI_ATTR VkResult vpCreateInstance(const VpInstanceCreateInfo *pCreateInfo,
                                     const VkAllocationCallbacks *pAllocator, VkInstance *pInstance) {
    VkInstanceCreateInfo createInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
    std::vector<const char*> extensions;
    VkInstanceCreateInfo* pInstanceCreateInfo = nullptr;

    if (pCreateInfo != nullptr && pCreateInfo->pCreateInfo != nullptr) {
        createInfo = *pCreateInfo->pCreateInfo;
        pInstanceCreateInfo = &createInfo;

        const detail::VpProfileDesc* pDesc = nullptr;
        if (pCreateInfo->pProfile != nullptr) {
            pDesc = detail::vpGetProfileDesc(pCreateInfo->pProfile->profileName);
            if (pDesc == nullptr) return VK_ERROR_UNKNOWN;
        }

        if (createInfo.pApplicationInfo == nullptr) {
            appInfo.apiVersion = pDesc->minApiVersion;
            createInfo.pApplicationInfo = &appInfo;
        }

        if (pDesc != nullptr && pDesc->pInstanceExtensions != nullptr) {
            bool merge = (pCreateInfo->flags & VP_INSTANCE_CREATE_MERGE_EXTENSIONS_BIT) != 0;
            bool override = (pCreateInfo->flags & VP_INSTANCE_CREATE_OVERRIDE_EXTENSIONS_BIT) != 0;

            if (!merge && !override && pCreateInfo->pCreateInfo->enabledExtensionCount > 0) {
                // If neither merge nor override is used then the application must not specify its
                // own extensions
                return VK_ERROR_UNKNOWN;
            }

            detail::vpGetExtensions(pCreateInfo->pCreateInfo->enabledExtensionCount,
                                    pCreateInfo->pCreateInfo->ppEnabledExtensionNames,
                                    pDesc->instanceExtensionCount,
                                    pDesc->pInstanceExtensions,
                                    extensions, merge, override);
            {
                bool foundPortEnum = false;
                for (size_t i = 0; i < extensions.size(); ++i) {
                    if (strcmp(extensions[i], VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0) {
                        foundPortEnum = true;
                        break;
                    }
                }
                if (foundPortEnum) {
                    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
                }
            }

            // Need to include VK_KHR_get_physical_device_properties2 if we are on Vulkan 1.0
            if (createInfo.pApplicationInfo->apiVersion < VK_API_VERSION_1_1) {
                bool foundGPDP2 = false;
                for (size_t i = 0; i < extensions.size(); ++i) {
                    if (strcmp(extensions[i], VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0) {
                        foundGPDP2 = true;
                        break;
                    }
                }
                if (!foundGPDP2) {
                    extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
                }
            }

            createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            createInfo.ppEnabledExtensionNames = extensions.data();
        }
    }

    return vkCreateInstance(pInstanceCreateInfo, pAllocator, pInstance);
}

VPAPI_ATTR VkResult vpGetPhysicalDeviceProfileSupport(VkInstance instance, VkPhysicalDevice physicalDevice,
                                                      const VpProfileProperties *pProfile, VkBool32 *pSupported) {
    VkResult result = VK_SUCCESS;

    uint32_t extCount = 0;
    result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
    if (result != VK_SUCCESS) {
        return result;
    }
    std::vector<VkExtensionProperties> ext;
    if (extCount > 0) {
        ext.resize(extCount);
    }
    result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, ext.data());
    if (result != VK_SUCCESS) {
        return result;
    }

    // Workaround old loader bug where count could be smaller on the second call to vkEnumerateDeviceExtensionProperties
    if (extCount > 0) {
        ext.resize(extCount);
    }

    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc == nullptr) return VK_ERROR_UNKNOWN;

    struct GPDP2EntryPoints {
        PFN_vkGetPhysicalDeviceFeatures2KHR                 pfnGetPhysicalDeviceFeatures2;
        PFN_vkGetPhysicalDeviceProperties2KHR               pfnGetPhysicalDeviceProperties2;
        PFN_vkGetPhysicalDeviceFormatProperties2KHR         pfnGetPhysicalDeviceFormatProperties2;
        PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR    pfnGetPhysicalDeviceQueueFamilyProperties2;
    };

    struct UserData {
        VkPhysicalDevice                    physicalDevice;
        const detail::VpProfileDesc*        pDesc;
        GPDP2EntryPoints                    gpdp2;
        uint32_t                            index;
        uint32_t                            count;
        detail::PFN_vpStructChainerCb       pfnCb;
        bool                                supported;
    } userData{ physicalDevice, pDesc };

    // Attempt to load core versions of the GPDP2 entry points
    userData.gpdp2.pfnGetPhysicalDeviceFeatures2 =
        (PFN_vkGetPhysicalDeviceFeatures2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures2");
    userData.gpdp2.pfnGetPhysicalDeviceProperties2 =
        (PFN_vkGetPhysicalDeviceProperties2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2");
    userData.gpdp2.pfnGetPhysicalDeviceFormatProperties2 =
        (PFN_vkGetPhysicalDeviceFormatProperties2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFormatProperties2");
    userData.gpdp2.pfnGetPhysicalDeviceQueueFamilyProperties2 =
        (PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties2");

    // If not successful, try to load KHR variant
    if (userData.gpdp2.pfnGetPhysicalDeviceFeatures2 == nullptr) {
        userData.gpdp2.pfnGetPhysicalDeviceFeatures2 =
            (PFN_vkGetPhysicalDeviceFeatures2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures2KHR");
        userData.gpdp2.pfnGetPhysicalDeviceProperties2 =
            (PFN_vkGetPhysicalDeviceProperties2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2KHR");
        userData.gpdp2.pfnGetPhysicalDeviceFormatProperties2 =
            (PFN_vkGetPhysicalDeviceFormatProperties2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFormatProperties2KHR");
        userData.gpdp2.pfnGetPhysicalDeviceQueueFamilyProperties2 =
            (PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties2KHR");
    }

    if (userData.gpdp2.pfnGetPhysicalDeviceFeatures2 == nullptr ||
        userData.gpdp2.pfnGetPhysicalDeviceProperties2 == nullptr ||
        userData.gpdp2.pfnGetPhysicalDeviceFormatProperties2 == nullptr ||
        userData.gpdp2.pfnGetPhysicalDeviceQueueFamilyProperties2 == nullptr) {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    *pSupported = VK_TRUE;
    VP_DEBUG_MSGF("Checking device support for profile %s (%s). You may find the details of the capabilities of this device on https://vulkan.gpuinfo.org/", pProfile->profileName, detail::vpGetDeviceAndDriverInfoString(physicalDevice, userData.gpdp2.pfnGetPhysicalDeviceProperties2).c_str());

    if (pDesc->props.specVersion < pProfile->specVersion) {
        VP_DEBUG_MSGF("Unsupported profile version: %u", pProfile->specVersion);
        *pSupported = VK_FALSE;
    }

    {
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(physicalDevice, &props);
        if (!detail::vpCheckVersion(props.apiVersion, pDesc->minApiVersion)) {
            VP_DEBUG_MSGF("Unsupported API version: %u.%u.%u", VK_API_VERSION_MAJOR(pDesc->minApiVersion), VK_API_VERSION_MINOR(pDesc->minApiVersion), VK_API_VERSION_PATCH(pDesc->minApiVersion));
            *pSupported = VK_FALSE;
        }
    }

    for (uint32_t i = 0; i < pDesc->deviceExtensionCount; ++i) {
        if (!detail::vpCheckExtension(ext.data(), ext.size(),
            pDesc->pDeviceExtensions[i].extensionName)) {
            *pSupported = VK_FALSE;
        }
    }

    {
        VkPhysicalDeviceFeatures2KHR features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR };
        pDesc->chainers.pfnFeature(static_cast<VkBaseOutStructure*>(static_cast<void*>(&features)), &userData,
            [](VkBaseOutStructure* p, void* pUser) {
                UserData* pUserData = static_cast<UserData*>(pUser);
                pUserData->gpdp2.pfnGetPhysicalDeviceFeatures2(pUserData->physicalDevice,
                                                               static_cast<VkPhysicalDeviceFeatures2KHR*>(static_cast<void*>(p)));
                pUserData->supported = true;
                while (p != nullptr) {
                    if (!pUserData->pDesc->feature.pfnComparator(p)) {
                        pUserData->supported = false;
                    }
                    p = p->pNext;
                }
            }
        );
        if (!userData.supported) {
            *pSupported = VK_FALSE;
        }
    }

    {
        VkPhysicalDeviceProperties2KHR props{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR };
        pDesc->chainers.pfnProperty(static_cast<VkBaseOutStructure*>(static_cast<void*>(&props)), &userData,
            [](VkBaseOutStructure* p, void* pUser) {
                UserData* pUserData = static_cast<UserData*>(pUser);
                pUserData->gpdp2.pfnGetPhysicalDeviceProperties2(pUserData->physicalDevice,
                                                                 static_cast<VkPhysicalDeviceProperties2KHR*>(static_cast<void*>(p)));
                pUserData->supported = true;
                while (p != nullptr) {
                    if (!pUserData->pDesc->property.pfnComparator(p)) {
                        pUserData->supported = false;
                    }
                    p = p->pNext;
                }
            }
        );
        if (!userData.supported) {
            *pSupported = VK_FALSE;
        }
    }

    for (uint32_t i = 0; i < pDesc->formatCount; ++i) {
        userData.index = i;
        VkFormatProperties2KHR props{ VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2_KHR };
        pDesc->chainers.pfnFormat(static_cast<VkBaseOutStructure*>(static_cast<void*>(&props)), &userData,
            [](VkBaseOutStructure* p, void* pUser) {
                UserData* pUserData = static_cast<UserData*>(pUser);
                pUserData->gpdp2.pfnGetPhysicalDeviceFormatProperties2(pUserData->physicalDevice,
                                                                       pUserData->pDesc->pFormats[pUserData->index].format,
                                                                       static_cast<VkFormatProperties2KHR*>(static_cast<void*>(p)));
                pUserData->supported = true;
                while (p != nullptr) {
                    if (!pUserData->pDesc->pFormats[pUserData->index].pfnComparator(p)) {
                        pUserData->supported = false;
                    }
                    p = p->pNext;
                }
            }
        );
        if (!userData.supported) {
            *pSupported = VK_FALSE;
        }
    }

    {
        userData.gpdp2.pfnGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &userData.count, nullptr);
        std::vector<VkQueueFamilyProperties2KHR> props(userData.count, { VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2_KHR });
        userData.index = 0;

        detail::PFN_vpStructChainerCb callback = [](VkBaseOutStructure* p, void* pUser) {
            UserData* pUserData = static_cast<UserData*>(pUser);
            VkQueueFamilyProperties2KHR* pProps = static_cast<VkQueueFamilyProperties2KHR*>(static_cast<void*>(p));
            if (++pUserData->index < pUserData->count) {
                pUserData->pDesc->chainers.pfnQueueFamily(static_cast<VkBaseOutStructure*>(static_cast<void*>(++pProps)),
                                                          pUser, pUserData->pfnCb);
            } else {
                pProps -= pUserData->count - 1;
                pUserData->gpdp2.pfnGetPhysicalDeviceQueueFamilyProperties2(pUserData->physicalDevice,
                                                                            &pUserData->count,
                                                                            pProps);
                pUserData->supported = true;

                // Check first that each queue family defined is supported by the device
                for (uint32_t i = 0; i < pUserData->pDesc->queueFamilyCount; ++i) {
                    bool found = false;
                    for (uint32_t j = 0; j < pUserData->count; ++j) {
                        bool propsMatch = true;
                        p = static_cast<VkBaseOutStructure*>(static_cast<void*>(&pProps[j]));
                        while (p != nullptr) {
                            if (!pUserData->pDesc->pQueueFamilies[i].pfnComparator(p)) {
                                propsMatch = false;
                                break;
                            }
                            p = p->pNext;
                        }
                        if (propsMatch) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        VP_DEBUG_MSGF("Unsupported queue family defined at profile data index #%u", i);
                        pUserData->supported = false;
                        return;
                    }
                }

                // Then check each permutation to ensure that while order of the queue families
                // doesn't matter, each queue family property criteria is matched with a separate
                // queue family of the actual device
                std::vector<uint32_t> permutation(pUserData->count);
                for (uint32_t i = 0; i < pUserData->count; ++i) {
                    permutation[i] = i;
                }
                bool found = false;
                do {
                    bool propsMatch = true;
                    for (uint32_t i = 0; i < pUserData->pDesc->queueFamilyCount && propsMatch; ++i) {
                        p = static_cast<VkBaseOutStructure*>(static_cast<void*>(&pProps[permutation[i]]));
                        while (p != nullptr) {
                            if (!pUserData->pDesc->pQueueFamilies[i].pfnComparator(p)) {
                                propsMatch = false;
                                break;
                            }
                            p = p->pNext;
                        }
                    }
                    if (propsMatch) {
                        found = true;
                        break;
                    }
                } while (std::next_permutation(permutation.begin(), permutation.end()));

                if (!found) {
                    VP_DEBUG_MSG("Unsupported combination of queue families");
                    pUserData->supported = false;
                }
            }
        };
        userData.pfnCb = callback;

        if (userData.count >= userData.pDesc->queueFamilyCount) {
            pDesc->chainers.pfnQueueFamily(static_cast<VkBaseOutStructure*>(static_cast<void*>(props.data())), &userData, callback);
            if (!userData.supported) {
                *pSupported = VK_FALSE;
            }
        } else {
            VP_DEBUG_MSGF("Unsupported number of queue families: device has fewer (%u) than what the profile defines (%u)", userData.count, userData.pDesc->queueFamilyCount);
            *pSupported = VK_FALSE;
        }
    }

    return result;
}

VPAPI_ATTR VkResult vpCreateDevice(VkPhysicalDevice physicalDevice, const VpDeviceCreateInfo *pCreateInfo,
                                   const VkAllocationCallbacks *pAllocator, VkDevice *pDevice) {
    if (physicalDevice == VK_NULL_HANDLE || pCreateInfo == nullptr || pDevice == nullptr) {
        return vkCreateDevice(physicalDevice, pCreateInfo == nullptr ? nullptr : pCreateInfo->pCreateInfo, pAllocator, pDevice);
    }

    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pCreateInfo->pProfile->profileName);
    if (pDesc == nullptr) return VK_ERROR_UNKNOWN;

    struct UserData {
        VkPhysicalDevice                physicalDevice;
        const detail::VpProfileDesc*    pDesc;
        const VpDeviceCreateInfo*       pCreateInfo;
        const VkAllocationCallbacks*    pAllocator;
        VkDevice*                       pDevice;
        VkResult                        result;
    } userData{ physicalDevice, pDesc, pCreateInfo, pAllocator, pDevice };

    VkPhysicalDeviceFeatures2KHR features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR };
    pDesc->chainers.pfnFeature(static_cast<VkBaseOutStructure*>(static_cast<void*>(&features)), &userData,
        [](VkBaseOutStructure* p, void* pUser) {
            UserData* pUserData = static_cast<UserData*>(pUser);
            const detail::VpProfileDesc* pDesc = pUserData->pDesc;
            const VpDeviceCreateInfo* pCreateInfo = pUserData->pCreateInfo;

            bool merge = (pCreateInfo->flags & VP_DEVICE_CREATE_MERGE_EXTENSIONS_BIT) != 0;
            bool override = (pCreateInfo->flags & VP_DEVICE_CREATE_OVERRIDE_EXTENSIONS_BIT) != 0;

            if (!merge && !override && pCreateInfo->pCreateInfo->enabledExtensionCount > 0) {
                // If neither merge nor override is used then the application must not specify its
                // own extensions
                pUserData->result = VK_ERROR_UNKNOWN;
                return;
            }

            std::vector<const char*> extensions;
            detail::vpGetExtensions(pCreateInfo->pCreateInfo->enabledExtensionCount,
                                    pCreateInfo->pCreateInfo->ppEnabledExtensionNames,
                                    pDesc->deviceExtensionCount,
                                    pDesc->pDeviceExtensions,
                                    extensions, merge, override);

            VkBaseOutStructure profileStructList;
            profileStructList.pNext = p;
            VkPhysicalDeviceFeatures2KHR* pFeatures = static_cast<VkPhysicalDeviceFeatures2KHR*>(static_cast<void*>(p));
            if (pDesc->feature.pfnFiller != nullptr) {
                while (p != nullptr) {
                    pDesc->feature.pfnFiller(p);
                    p = p->pNext;
                }
            }

            if (pCreateInfo->pCreateInfo->pEnabledFeatures != nullptr) {
                pFeatures->features = *pCreateInfo->pCreateInfo->pEnabledFeatures;
            }

            if (pCreateInfo->flags & VP_DEVICE_CREATE_DISABLE_ROBUST_BUFFER_ACCESS_BIT) {
                pFeatures->features.robustBufferAccess = VK_FALSE;
            }

#ifdef VK_EXT_robustness2
            VkPhysicalDeviceRobustness2FeaturesEXT* pRobustness2FeaturesEXT = static_cast<VkPhysicalDeviceRobustness2FeaturesEXT*>(
                detail::vpGetStructure(pFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT));
            if (pRobustness2FeaturesEXT != nullptr) {
                if (pCreateInfo->flags & VP_DEVICE_CREATE_DISABLE_ROBUST_BUFFER_ACCESS_BIT) {
                    pRobustness2FeaturesEXT->robustBufferAccess2 = VK_FALSE;
                }
                if (pCreateInfo->flags & VP_DEVICE_CREATE_DISABLE_ROBUST_IMAGE_ACCESS_BIT) {
                    pRobustness2FeaturesEXT->robustImageAccess2 = VK_FALSE;
                }
            }
#endif

#ifdef VK_EXT_image_robustness
            VkPhysicalDeviceImageRobustnessFeaturesEXT* pImageRobustnessFeaturesEXT = static_cast<VkPhysicalDeviceImageRobustnessFeaturesEXT*>(
                detail::vpGetStructure(pFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_ROBUSTNESS_FEATURES_EXT));
            if (pImageRobustnessFeaturesEXT != nullptr && (pCreateInfo->flags & VP_DEVICE_CREATE_DISABLE_ROBUST_IMAGE_ACCESS_BIT)) {
                pImageRobustnessFeaturesEXT->robustImageAccess = VK_FALSE;
            }
#endif

#ifdef VK_VERSION_1_3
            VkPhysicalDeviceVulkan13Features* pVulkan13Features = static_cast<VkPhysicalDeviceVulkan13Features*>(
                detail::vpGetStructure(pFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES));
            if (pVulkan13Features != nullptr && (pCreateInfo->flags & VP_DEVICE_CREATE_DISABLE_ROBUST_IMAGE_ACCESS_BIT)) {
                pVulkan13Features->robustImageAccess = VK_FALSE;
            }
#endif

            VkBaseOutStructure* pNext = static_cast<VkBaseOutStructure*>(const_cast<void*>(pCreateInfo->pCreateInfo->pNext));
            if ((pCreateInfo->flags & VP_DEVICE_CREATE_OVERRIDE_ALL_FEATURES_BIT) == 0) {
                for (uint32_t i = 0; i < pDesc->featureStructTypeCount; ++i) {
                    const void* pRequested = detail::vpGetStructure(pNext, pDesc->pFeatureStructTypes[i]);
                    if (pRequested == nullptr) {
                        VkBaseOutStructure* pPrevStruct = &profileStructList;
                        VkBaseOutStructure* pCurrStruct = pPrevStruct->pNext;
                        while (pCurrStruct->sType != pDesc->pFeatureStructTypes[i]) {
                            pPrevStruct = pCurrStruct;
                            pCurrStruct = pCurrStruct->pNext;
                        }
                        pPrevStruct->pNext = pCurrStruct->pNext;
                        pCurrStruct->pNext = pNext;
                        pNext = pCurrStruct;
                    } else
                    if ((pCreateInfo->flags & VP_DEVICE_CREATE_OVERRIDE_FEATURES_BIT) == 0) {
                        // If override is not used then the application must not specify its
                        // own feature structure for anything that the profile defines
                        pUserData->result = VK_ERROR_UNKNOWN;
                        return;
                    }
                }
            }

            VkDeviceCreateInfo createInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
            createInfo.pNext = pNext;
            createInfo.queueCreateInfoCount = pCreateInfo->pCreateInfo->queueCreateInfoCount;
            createInfo.pQueueCreateInfos = pCreateInfo->pCreateInfo->pQueueCreateInfos;
            createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            createInfo.ppEnabledExtensionNames = extensions.data();
            if (pCreateInfo->flags & VP_DEVICE_CREATE_OVERRIDE_ALL_FEATURES_BIT) {
                createInfo.pEnabledFeatures = pCreateInfo->pCreateInfo->pEnabledFeatures;
            }
            pUserData->result = vkCreateDevice(pUserData->physicalDevice, &createInfo, pUserData->pAllocator, pUserData->pDevice);
        }
    );

    return userData.result;
}

VPAPI_ATTR VkResult vpGetProfileInstanceExtensionProperties(const VpProfileProperties *pProfile, uint32_t *pPropertyCount,
                                                            VkExtensionProperties *pProperties) {
    VkResult result = VK_SUCCESS;

    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc == nullptr) return VK_ERROR_UNKNOWN;

    if (pProperties == nullptr) {
        *pPropertyCount = pDesc->instanceExtensionCount;
    } else {
        if (*pPropertyCount < pDesc->instanceExtensionCount) {
            result = VK_INCOMPLETE;
        } else {
            *pPropertyCount = pDesc->instanceExtensionCount;
        }
        for (uint32_t i = 0; i < *pPropertyCount; ++i) {
            pProperties[i] = pDesc->pInstanceExtensions[i];
        }
    }
    return result;
}

VPAPI_ATTR VkResult vpGetProfileDeviceExtensionProperties(const VpProfileProperties *pProfile, uint32_t *pPropertyCount,
                                                          VkExtensionProperties *pProperties) {
    VkResult result = VK_SUCCESS;

    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc == nullptr) return VK_ERROR_UNKNOWN;

    if (pProperties == nullptr) {
        *pPropertyCount = pDesc->deviceExtensionCount;
    } else {
        if (*pPropertyCount < pDesc->deviceExtensionCount) {
            result = VK_INCOMPLETE;
        } else {
            *pPropertyCount = pDesc->deviceExtensionCount;
        }
        for (uint32_t i = 0; i < *pPropertyCount; ++i) {
            pProperties[i] = pDesc->pDeviceExtensions[i];
        }
    }
    return result;
}

VPAPI_ATTR void vpGetProfileFeatures(const VpProfileProperties *pProfile, void *pNext) {
    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc != nullptr && pDesc->feature.pfnFiller != nullptr) {
        VkBaseOutStructure* p = static_cast<VkBaseOutStructure*>(pNext);
        while (p != nullptr) {
            pDesc->feature.pfnFiller(p);
            p = p->pNext;
        }
    }
}

VPAPI_ATTR VkResult vpGetProfileFeatureStructureTypes(const VpProfileProperties *pProfile, uint32_t *pStructureTypeCount,
                                                      VkStructureType *pStructureTypes) {
    VkResult result = VK_SUCCESS;

    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc == nullptr) return VK_ERROR_UNKNOWN;

    if (pStructureTypes == nullptr) {
        *pStructureTypeCount = pDesc->featureStructTypeCount;
    } else {
        if (*pStructureTypeCount < pDesc->featureStructTypeCount) {
            result = VK_INCOMPLETE;
        } else {
            *pStructureTypeCount = pDesc->featureStructTypeCount;
        }
        for (uint32_t i = 0; i < *pStructureTypeCount; ++i) {
            pStructureTypes[i] = pDesc->pFeatureStructTypes[i];
        }
    }
    return result;
}

VPAPI_ATTR void vpGetProfileProperties(const VpProfileProperties *pProfile, void *pNext) {
    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc != nullptr && pDesc->property.pfnFiller != nullptr) {
        VkBaseOutStructure* p = static_cast<VkBaseOutStructure*>(pNext);
        while (p != nullptr) {
            pDesc->property.pfnFiller(p);
            p = p->pNext;
        }
    }
}

VPAPI_ATTR VkResult vpGetProfilePropertyStructureTypes(const VpProfileProperties *pProfile, uint32_t *pStructureTypeCount,
                                                       VkStructureType *pStructureTypes) {
    VkResult result = VK_SUCCESS;

    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc == nullptr) return VK_ERROR_UNKNOWN;

    if (pStructureTypes == nullptr) {
        *pStructureTypeCount = pDesc->propertyStructTypeCount;
    } else {
        if (*pStructureTypeCount < pDesc->propertyStructTypeCount) {
            result = VK_INCOMPLETE;
        } else {
            *pStructureTypeCount = pDesc->propertyStructTypeCount;
        }
        for (uint32_t i = 0; i < *pStructureTypeCount; ++i) {
            pStructureTypes[i] = pDesc->pPropertyStructTypes[i];
        }
    }
    return result;
}

VPAPI_ATTR VkResult vpGetProfileQueueFamilyProperties(const VpProfileProperties *pProfile, uint32_t *pPropertyCount,
                                                      VkQueueFamilyProperties2KHR *pProperties) {
    VkResult result = VK_SUCCESS;

    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc == nullptr) return VK_ERROR_UNKNOWN;

    if (pProperties == nullptr) {
        *pPropertyCount = pDesc->queueFamilyCount;
    } else {
        if (*pPropertyCount < pDesc->queueFamilyCount) {
            result = VK_INCOMPLETE;
        } else {
            *pPropertyCount = pDesc->queueFamilyCount;
        }
        for (uint32_t i = 0; i < *pPropertyCount; ++i) {
            VkBaseOutStructure* p = static_cast<VkBaseOutStructure*>(static_cast<void*>(&pProperties[i]));
            while (p != nullptr) {
                pDesc->pQueueFamilies[i].pfnFiller(p);
                p = p->pNext;
            }
        }
    }
    return result;
}

VPAPI_ATTR VkResult vpGetProfileQueueFamilyStructureTypes(const VpProfileProperties *pProfile, uint32_t *pStructureTypeCount,
                                                          VkStructureType *pStructureTypes) {
    VkResult result = VK_SUCCESS;

    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc == nullptr) return VK_ERROR_UNKNOWN;

    if (pStructureTypes == nullptr) {
        *pStructureTypeCount = pDesc->queueFamilyStructTypeCount;
    } else {
        if (*pStructureTypeCount < pDesc->queueFamilyStructTypeCount) {
            result = VK_INCOMPLETE;
        } else {
            *pStructureTypeCount = pDesc->queueFamilyStructTypeCount;
        }
        for (uint32_t i = 0; i < *pStructureTypeCount; ++i) {
            pStructureTypes[i] = pDesc->pQueueFamilyStructTypes[i];
        }
    }
    return result;
}

VPAPI_ATTR VkResult vpGetProfileFormats(const VpProfileProperties *pProfile, uint32_t *pFormatCount, VkFormat *pFormats) {
    VkResult result = VK_SUCCESS;

    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc == nullptr) return VK_ERROR_UNKNOWN;

    if (pFormats == nullptr) {
        *pFormatCount = pDesc->formatCount;
    } else {
        if (*pFormatCount < pDesc->formatCount) {
            result = VK_INCOMPLETE;
        } else {
            *pFormatCount = pDesc->formatCount;
        }
        for (uint32_t i = 0; i < *pFormatCount; ++i) {
            pFormats[i] = pDesc->pFormats[i].format;
        }
    }
    return result;
}

VPAPI_ATTR void vpGetProfileFormatProperties(const VpProfileProperties *pProfile, VkFormat format, void *pNext) {
    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc == nullptr) return;

    for (uint32_t i = 0; i < pDesc->formatCount; ++i) {
        if (pDesc->pFormats[i].format == format) {
            VkBaseOutStructure* p = static_cast<VkBaseOutStructure*>(static_cast<void*>(pNext));
            while (p != nullptr) {
                pDesc->pFormats[i].pfnFiller(p);
                p = p->pNext;
            }
#if defined(VK_VERSION_1_3) || defined(VK_KHR_format_feature_flags2)
            VkFormatProperties2KHR* fp2 = static_cast<VkFormatProperties2KHR*>(
                detail::vpGetStructure(pNext, VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2_KHR));
            VkFormatProperties3KHR* fp3 = static_cast<VkFormatProperties3KHR*>(
                detail::vpGetStructure(pNext, VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_3_KHR));
            if (fp3 != nullptr) {
                VkFormatProperties2KHR fp{ VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2_KHR };
                pDesc->pFormats[i].pfnFiller(static_cast<VkBaseOutStructure*>(static_cast<void*>(&fp)));
                fp3->linearTilingFeatures = static_cast<VkFormatFeatureFlags2KHR>(fp3->linearTilingFeatures | fp.formatProperties.linearTilingFeatures);
                fp3->optimalTilingFeatures = static_cast<VkFormatFeatureFlags2KHR>(fp3->optimalTilingFeatures | fp.formatProperties.optimalTilingFeatures);
                fp3->bufferFeatures = static_cast<VkFormatFeatureFlags2KHR>(fp3->bufferFeatures | fp.formatProperties.bufferFeatures);
            }
            if (fp2 != nullptr) {
                VkFormatProperties3KHR fp{ VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_3_KHR };
                pDesc->pFormats[i].pfnFiller(static_cast<VkBaseOutStructure*>(static_cast<void*>(&fp)));
                fp2->formatProperties.linearTilingFeatures = static_cast<VkFormatFeatureFlags>(fp2->formatProperties.linearTilingFeatures | fp.linearTilingFeatures);
                fp2->formatProperties.optimalTilingFeatures = static_cast<VkFormatFeatureFlags>(fp2->formatProperties.optimalTilingFeatures | fp.optimalTilingFeatures);
                fp2->formatProperties.bufferFeatures = static_cast<VkFormatFeatureFlags>(fp2->formatProperties.bufferFeatures | fp.bufferFeatures);
            }
#endif
        }
    }
}

VPAPI_ATTR VkResult vpGetProfileFormatStructureTypes(const VpProfileProperties *pProfile, uint32_t *pStructureTypeCount,
                                                     VkStructureType *pStructureTypes) {
    VkResult result = VK_SUCCESS;

    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc == nullptr) return VK_ERROR_UNKNOWN;

    if (pStructureTypes == nullptr) {
        *pStructureTypeCount = pDesc->formatStructTypeCount;
    } else {
        if (*pStructureTypeCount < pDesc->formatStructTypeCount) {
            result = VK_INCOMPLETE;
        } else {
            *pStructureTypeCount = pDesc->formatStructTypeCount;
        }
        for (uint32_t i = 0; i < *pStructureTypeCount; ++i) {
            pStructureTypes[i] = pDesc->pFormatStructTypes[i];
        }
    }
    return result;
}
'''


class Log():
    def f(msg):
        print('FATAL: ' + msg)
        raise Exception(msg)

    def e(msg):
        print('ERROR: ' + msg)

    def w(msg):
        print('WARNING: ' + msg)

    def i(msg):
        print(msg)


class VulkanPlatform():
    def __init__(self, data):
        self.name = data.get('name')
        self.protect = data.get('protect')


class VulkanStructMember():
    def __init__(self, name, type, limittype, isArray = False):
        self.name = name
        self.type = type
        self.limittype = limittype
        self.isArray = isArray
        self.arraySizeMember = None
        self.nullTerminated = False
        self.arraySize = None


class VulkanStruct():
    def __init__(self, name):
        self.name = name
        self.sType = None
        self.extends = []
        self.members = OrderedDict()
        self.aliases = [ name ]
        self.isAlias = False
        self.definedByVersion = None
        self.definedByExtensions = []


class VulkanEnum():
    def __init__(self, name):
        self.name = name
        self.aliases = [ name ]
        self.isAlias = False
        self.values = []
        self.aliasValues = dict()


class VulkanBitmask():
    def __init__(self, name):
        self.name = name
        self.aliases = [ name ]
        self.isAlias = False
        self.bitsType = None


class VulkanFeature():
    def __init__(self, name):
        self.name = name
        self.structs = set()


class VulkanLimit():
    def __init__(self, name):
        self.name = name
        self.structs = set()


class VulkanVersionNumber():
    def __init__(self, versionStr):
        match = re.search(r"^([1-9][0-9]*)\.([0-9]+)$", versionStr)
        if match != None:
            # Only major and minor version specified
            self.major = int(match.group(1))
            self.minor = int(match.group(2))
            self.patch = None
        else:
            # Otherwise expect major, minor, and patch version
            match = re.search(r"^([1-9][0-9]*)\.([0-9]+)\.([0-9]+)$", versionStr)
            if match != None:
                self.major = int(match.group(1))
                self.minor = int(match.group(2))
                self.patch = int(match.group(3))
            else:
                Log.f("Invalid API version string: '{0}'".format(versionStr))

        # Construct version number pre-processor definition's name
        self.define = 'VK_VERSION_{0}_{1}'.format(self.major, self.minor)

    def get_api_version_string(self):
        return 'VK_API_VERSION_' + str(self.major) + '_' + str(self.minor)

    def __eq__(self, other):
        if isinstance(other, VulkanVersionNumber):
            # Only consider major and minor version in comparison
            return self.major == other.major and self.minor == other.minor
        else:
            return False

    def __gt__(self, other):
        # Only consider major and minor version in comparison
        return self.major > other.major or (self.major == other.major and self.minor > other.minor)

    def __lt__(self, other):
        # Only consider major and minor version in comparison
        return self.major < other.major or (self.major == other.major and self.minor < other.minor)

    def __ne__(self, other):
        return not self.__eq__(other)

    def __ge__(self, other):
        return self.__eq__(other) or self.__gt__(other)

    def __le__(self, other):
        return self.__eq__(other) or self.__lt__(other)

    def __str__(self):
        if self.patch != None:
            return '{0}.{1}.{2}'.format(self.major, self.minor, self.patch)
        else:
            return '{0}.{1}'.format(self.major, self.minor)


class VulkanDefinitionScope():
    def parseAliases(self, xml):
        self.sTypeAliases = dict()
        for sTypeAlias in xml.findall("./require/enum[@alias]"):
            if re.search(r'^VK_STRUCTURE_TYPE_.*', sTypeAlias.get('name')):
                self.sTypeAliases[sTypeAlias.get('alias')] = sTypeAlias.get('name')


class VulkanVersion(VulkanDefinitionScope):
    def __init__(self, xml):
        self.name = xml.get('name')
        self.number = VulkanVersionNumber(xml.get('number'))
        self.extensions = []
        self.features = dict()
        self.limits = dict()
        self.parseAliases(xml)


class VulkanExtension(VulkanDefinitionScope):
    def __init__(self, xml, upperCaseName):
        self.name = xml.get('name')
        self.upperCaseName = upperCaseName
        self.type = xml.get('type')
        self.features = dict()
        self.limits = dict()
        self.platform = xml.get('platform')
        self.provisional = xml.get('provisional')
        self.promotedTo = xml.get('promotedto')
        self.obsoletedBy = xml.get('obsoletedby')
        self.deprecatedBy = xml.get('deprecatedby')
        self.spec_version = 1
        for e in xml.findall("./require/enum"):
            if (e.get('name').endswith("SPEC_VERSION")):
                self.spec_version = e.get('value')
                break
        self.parseAliases(xml)


class VulkanRegistry():
    def __init__(self, registryFile):
        Log.i("Loading registry file: '{0}'".format(registryFile))
        xml = etree.parse(registryFile)
        stripNonmatchingAPIs(xml.getroot(), 'vulkan', actuallyDelete = True)

        self.parsePlatformInfo(xml)
        self.parseVersionInfo(xml)
        self.parseExtensionInfo(xml)
        self.parseStructInfo(xml)
        self.parsePrerequisites(xml)
        self.parseEnums(xml)
        self.parseFormats(xml)
        self.parseBitmasks(xml)
        self.parseConstants(xml)
        self.parseAliases(xml)
        self.parseExternalTypes(xml)
        self.parseFeatures(xml)
        self.parseLimits(xml)
        self.parseHeaderVersion(xml)
        self.applyWorkarounds()


    def parsePlatformInfo(self, xml):
        self.platforms = dict()
        for plat in xml.findall("./platforms/platform"):
            self.platforms[plat.get('name')] = VulkanPlatform(plat)


    def parseVersionInfo(self, xml):
        self.versions = dict()
        for feature in xml.findall("./feature[@api='vulkan']") + xml.findall("./feature[@api='vulkan,vulkansc']"):
            if re.search(r"^[1-9][0-9]*\.[0-9]+$", feature.get('number')):
                self.versions[feature.get('name')] = VulkanVersion(feature)
            else:
                Log.f("Unsupported feature with number '{0}'".format(feature.get('number')))


    def parseExtensionInfo(self, xml):
        self.extensions = dict()
        for ext in xml.findall("./extensions/extension[@supported='vulkan']") + xml.findall("./extensions/extension[@supported='vulkan,vulkansc']"):
            name = ext.get('name')

            # Find name enum (due to inconsistencies in lower case and upper case names this is non-trivial)
            foundNameEnum = False
            matches = ext.findall("./require/enum[@value='\"" + name + "\"']")
            for match in matches:
                if match.get('name').endswith("_EXTENSION_NAME"):
                    # Add extension definition
                    self.extensions[name] = VulkanExtension(ext, match.get('name')[:-len("_EXTENSION_NAME")])
                    foundNameEnum = True
                    break
            if not foundNameEnum:
                Log.f("Cannot find name enum for extension '{0}'".format(name))


    def isVulkanscStruct(self, xml, structName):
        if (structName == 'VkPhysicalDeviceVulkanSC10Features'):
            return True
        if (structName == 'VkPhysicalDeviceVulkanSC10Properties'):
            return True
        for extension in xml.findall("./extensions/extension[@supported='vulkansc']"):
            for requireType in extension.findall('./require/type'):
                if requireType.get('name') == structName:
                    return True
        return False

    def parseStructInfo(self, xml):
        self.structs = dict()
        for struct in xml.findall("./types/type[@category='struct']"):
            if self.isVulkanscStruct(xml, struct.get('name')):
                continue

            # Define base struct information
            structDef = VulkanStruct(struct.get('name'))

            # Find out whether it's an extension structure
            extends = struct.get('structextends')
            if extends != None:
                structDef.extends = extends.split(',')

            # Find sType value
            sType = struct.find("./member[name='sType']")
            if sType != None:
                structDef.sType = sType.get('values')

            # Parse struct members
            for member in struct.findall('./member'):
                name = member.find('./name').text
                tail = member.find('./name').tail
                type = member.find('./type').text

                # Only add real members (skip sType and pNext)
                if name != 'sType' and name != 'pNext':
                    # Define base member information
                    structDef.members[name] = VulkanStructMember(
                        name,
                        type,
                        member.get('limittype')
                    )

                    # Detect if it's an array
                    if tail != None and tail[0] == '[':
                        structDef.members[name].isArray = True
                        match1D = re.search(r"^\[([0-9]+)\]$", tail)
                        match2D = re.search(r"^\[([0-9]+)\]\[([0-9]+)\]$", tail)
                        enum = member.find('./enum')
                        if match1D != None:
                            # [<number>] case
                            structDef.members[name].arraySize = int(match1D.group(1))
                        elif match2D != None:
                            # [<number>][<number>] case
                            structDef.members[name].arraySize = [ int(match2D.group(1)), int(match2D.group(2)) ]
                        elif tail == '[' and enum != None and enum.tail == ']':
                            # [<enum>] case
                            structDef.members[name].arraySize = enum.text
                        else:
                            Log.f("Unsupported array format for struct member '{0}::{1}'".format(structDef.name, name))

                    # If it has a "len" attribute then it's also an array, just a dynamically sized one
                    if member.get('len') != None:
                        lenMeta = member.get('len').split(',')
                        for len in lenMeta:
                            if len == 'null-terminated':
                                # Values are null-terminated
                                structDef.members[name].nullTerminated = True
                            else:
                                # This is a pointer to an array with a corresponding count member
                                structDef.members[name].isArray = True
                                structDef.members[name].arraySizeMember = len

            # If any of the members is a dynamic array then we should remove the corresponding count member
            for member in list(structDef.members.values()):
                if member.isArray and member.arraySizeMember != None:
                    structDef.members.pop(member.arraySizeMember, None)

            # Store struct definition
            self.structs[struct.get('name')] = structDef


    def parsePrerequisites(self, xml):
        # Check features (i.e. API versions)
        for feature in xml.findall("./feature[@api='vulkan']") + xml.findall("./feature[@api='vulkan,vulkansc']"):
            for requireType in feature.findall('./require/type'):
                # Add feature as the source of the definition of a struct
                if requireType.get('name') in self.structs:
                    self.structs[requireType.get('name')].definedByVersion = VulkanVersionNumber(feature.get('number'))

        # Check extensions
        for extension in xml.findall("./extensions/extension[@supported='vulkan']") + xml.findall("./extensions/extension[@supported='vulkan,vulkansc']"):
            for requireType in extension.findall('./require/type'):
                # Add extension as the source of the definition of a struct
                if requireType.get('name') in self.structs:
                    self.structs[requireType.get('name')].definedByExtensions.append(extension.get('name'))


    def parseEnums(self, xml):
        self.enums = dict()
        # Find enum definitions
        for enum in xml.findall("./types/type[@category='enum']"):
            # Create enum type
            enumDef = VulkanEnum(enum.get('name'))

            # First collect base values
            values = xml.find("./enums[@name='" + enumDef.name + "']")
            if values:
                for value in values.findall("./enum"):
                    if value.get('alias') is None:
                        enumDef.values.append(value.get('name'))

            # Then find extension values
            for value in xml.findall(".//feature[@api='vulkan']/require/enum[@extends='" + enumDef.name + "']") + xml.findall(".//feature[@api='vulkan,vulkansc']/require/enum[@extends='" + enumDef.name + "']"):
                if value.get('alias') is None:
                    enumDef.values.append(value.get('name'))
            for value in xml.findall(".//extension[@supported='vulkan']/require/enum[@extends='" + enumDef.name + "']") + xml.findall(".//extension[@supported='vulkan,vulkansc']/require/enum[@extends='" + enumDef.name + "']"):
                if value.get('alias') is None:
                    enumDef.values.append(value.get('name'))

            # Finally store it in the registry
            self.enums[enumDef.name] = enumDef


    def parseFormats(self, xml):
        self.formatCompression = dict()
        for enum in xml.findall("./formats/format"):
            if enum.get('compressed'):
                self.formatCompression[enum.get('name')] = enum.get('compressed')

        self.aliasFormats = list()
        for format in xml.findall("./extensions/extension[@supported='vulkan']/require/enum[@extends='VkFormat'][@alias]") + xml.findall("./extensions/extension[@supported='vulkan,vulkansc']/require/enum[@extends='VkFormat'][@alias]"):
            self.aliasFormats.append(format.attrib["name"])

        self.betaFormatFeatures = list()
        for format_feature in xml.findall("./extensions/extension[@supported='vulkan']/require/enum[@protect='VK_ENABLE_BETA_EXTENSIONS']") + xml.findall("./extensions/extension[@supported='vulkan,vulkansc']/require/enum[@protect='VK_ENABLE_BETA_EXTENSIONS']"):
            self.betaFormatFeatures.append(format_feature.attrib["name"])

    def parseBitmasks(self, xml):
        self.bitmasks = dict()
        # Find bitmask definitions
        for bitmask in xml.findall("./types/type[@category='bitmask']"):
            # Only consider non-alias bitmasks
            name = bitmask.find("./name")
            if bitmask.get('alias') is None and name != None:
                bitmaskDef = VulkanBitmask(name.text)

                # Get the name of the corresponding FlagBits type
                bitsName = bitmask.get('bitvalues')
                if bitsName is None:
                    # Currently some definitions use "requires", not "bitvalues"
                    bitsName = bitmask.get('requires')

                if bitsName != None:
                    if bitsName in self.enums:
                        bitmaskDef.bitsType = self.enums[bitsName]
                    else:
                        Log.f("Could not find bits enum '{0}' for bitmask '{1}'", bitsName, bitmaskDef.name)
                else:
                    # This bitmask doesn't have any bits defined
                    pass

                # Finally store it in the registry
                self.bitmasks[bitmaskDef.name] = bitmaskDef


    def parseConstants(self, xml):
        self.constants = dict()
        # Find constant definitions
        constants = xml.find("./enums[@name='API Constants']").findall("./enum[@value]")
        if constants != None:
            for constant in constants:
                self.constants[constant.get('name')] = constant.get('value')
        else:
            Log.f("Failed to find API constants in the registry")


    def parseAliases(self, xml):
        # Find any struct aliases
        for struct in xml.findall("./types/type[@category='struct']"):
            alias = struct.get('alias')
            if alias != None:
                if alias in self.structs:
                    baseStructDef = self.structs[alias]
                    aliasStructDef = self.structs[struct.get('name')]

                    # Set as alias
                    aliasStructDef.isAlias = True

                    # Fill missing struct information for the alias
                    aliasStructDef.extends = baseStructDef.extends
                    aliasStructDef.members = baseStructDef.members
                    aliasStructDef.aliases = baseStructDef.aliases
                    aliasStructDef.aliases.append(struct.get('name'))

                    if baseStructDef.sType != None:
                        sTypeAlias = None

                        # First try to find sType alias in core versions
                        if aliasStructDef.definedByVersion != None:
                            for versionName in self.versions:
                                version = self.versions[versionName]
                                if version.number <= aliasStructDef.definedByVersion:
                                    sTypeAlias = version.sTypeAliases.get(baseStructDef.sType)
                                    if sTypeAlias != None:
                                        break

                        # Otherwise need to find sType alias in extension
                        if sTypeAlias == None:
                            for extName in aliasStructDef.definedByExtensions:
                                sTypeAlias = self.extensions[extName].sTypeAliases.get(baseStructDef.sType)
                                if sTypeAlias != None:
                                    break

                        #Workaround due to a vk.xml issue that was resolved with 1.1.119
                        if alias == 'VkPhysicalDeviceVariablePointersFeatures':
                            sTypeAlias = 'VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTERS_FEATURES'
                        
                        if sTypeAlias != None:
                            aliasStructDef.sType = sTypeAlias


        # Find any enum aliases
        for enum in xml.findall("./types/type[@category='enum']"):
            alias = enum.get('alias')
            if alias != None:
                if alias in self.enums:
                    baseEnumDef = self.enums[alias]
                    aliasEnumDef = self.enums[enum.get('name')]

                    # Set as alias
                    aliasEnumDef.isAlias = True

                    # Merge aliases
                    aliasEnumDef.aliases = baseEnumDef.aliases
                    aliasEnumDef.aliases.append(enum.get('name'))

                    # Merge values respecting original order
                    for value in aliasEnumDef.values:
                        if not value in baseEnumDef.values:
                            baseEnumDef.values.append(value)
                    aliasEnumDef.values = baseEnumDef.values
                else:
                    Log.f("Failed to find alias '{0}' of enum '{1}'".format(alias, enum.get('name')))

        # Find any enum value aliases
        for enum in xml.findall("./enums"):
            if enum.get('name') in self.enums.keys():
                enumDef = self.enums[enum.get('name')]
                for aliasValue in enum.findall("./enum[@alias]"):
                    name = aliasValue.get('name')
                    alias = aliasValue.get('alias')
                    enumDef.values.append(name)
                    enumDef.aliasValues[name] = alias
        for aliasValue in xml.findall("./extensions/extension[@supported='vulkan']/require/enum[@alias]") + xml.findall("./extensions/extension[@supported='vulkan,vulkansc']/require/enum[@alias]"):
            if aliasValue.get('extends'):
                enumDef = self.enums[aliasValue.get('extends')]
                name = aliasValue.get('name')
                alias = aliasValue.get('alias')
                enumDef.values.append(name)
                enumDef.aliasValues[name] = alias

        # Find any bitmask (flags) aliases
        for bitmask in xml.findall("./types/type[@category='bitmask']"):
            name = bitmask.get('name')
            alias = bitmask.get('alias')
            if alias != None:
                if alias in self.bitmasks:
                    # Duplicate bitmask definition
                    baseBitmaskDef = self.bitmasks[alias]
                    aliasBitmaskDef = VulkanBitmask(name)
                    aliasBitmaskDef.bitsType = baseBitmaskDef.bitsType

                    # Set as alias
                    aliasBitmaskDef.isAlias = True

                    # Merge aliases
                    aliasBitmaskDef.aliases = baseBitmaskDef.aliases
                    aliasBitmaskDef.aliases.append(name)
                else:
                    Log.f("Failed to find alias '{0}' of bitmask '{1}'".format(alias, bitmask.get('name')))

        # Find any constant aliases
        for constant in xml.find("./enums[@name='API Constants']").findall("./enum[@alias]"):
            self.constants[constant.get('name')] = self.constants[constant.get('alias')]


    def parseExternalTypes(self, xml):
        self.includes = set()
        self.externalTypes = set()

        # Find all include definitions
        for include in xml.findall("./types/type[@category='include']"):
            self.includes.add(include.get('name'))

        # Find all types depending on the includes
        for type in xml.findall("./types/type[@requires]"):
            if type.get('requires') in self.includes:
                self.externalTypes.add(type.get('name'))

    def parseFeatures(self, xml):
        # First, parse features specific to Vulkan versions
        for version in self.versions.values():
            if version.name == 'VK_VERSION_1_0':
                # For version 1.0 use VkPhysicalDeviceFeatures
                structDef = self.structs['VkPhysicalDeviceFeatures']
                for memberDef in structDef.members.values():
                    version.features[memberDef.name] = VulkanFeature(memberDef.name)
                    version.features[memberDef.name].structs.add('VkPhysicalDeviceFeatures')
            else:
                # For all other versions use the feature structures required by it
                featureStructNames = []
                xmlVersion = xml.find("./feature[@name='" + version.name + "']")
                for type in xmlVersion.findall("./require/type"):
                    name = type.get('name')
                    if name in self.structs and 'VkPhysicalDeviceFeatures2' in self.structs[name].extends:
                        featureStructNames.append(name)
                # VkPhysicalDeviceVulkan11Features is defined in Vulkan 1.2, but it actually
                # contains Vulkan 1.1 features, so treat it as such
                if version.name == 'VK_VERSION_1_1':
                    featureStructNames.append('VkPhysicalDeviceVulkan11Features')
                elif version.name == 'VK_VERSION_1_2':
                    featureStructNames.remove('VkPhysicalDeviceVulkan11Features')
                # For each feature collect all feature structures containing them, and their aliases
                for featureStructName in featureStructNames:
                    if (featureStructName in self.structs):
                        structDef = self.structs[featureStructName]
                        for memberName in structDef.members.keys():
                            if not memberName in version.features:
                                version.features[memberName] = VulkanFeature(memberName)
                            version.features[memberName].structs.update(structDef.aliases)

        # Then parse features specific to extensions
        for extension in self.extensions.values():
            featureStructNames = []
            xmlExtension = xml.find("./extensions/extension[@name='" + extension.name + "']")
            for type in xmlExtension.findall("./require/type"):
                name = type.get('name')
                if name in self.structs and 'VkPhysicalDeviceFeatures2' in self.structs[name].extends:
                    featureStructNames.append(name)
            # For each feature collect all feature structures containing them, and their aliases
            for featureStructName in featureStructNames:
                structDef = self.structs[featureStructName]
                for memberName in structDef.members.keys():
                    extension.features[memberName] = VulkanFeature(memberName)
                    extension.features[memberName].structs.update(structDef.aliases)
                    # For each feature we also have to check whether it's part of core so that
                    # any not strictly alias struct (i.e. the VkPhysicalDeviceVulkanXXFeatures)
                    # get included as well
                    for version in self.versions.values():
                        if memberName in version.features and version.features[memberName].structs >= extension.features[memberName].structs:
                            extension.features[memberName].structs = version.features[memberName].structs


    def parseLimits(self, xml):
        # First, parse properties/limits specific to Vulkan versions
        for version in self.versions.values():
            if version.name == 'VK_VERSION_1_0':
                # The properties extension structures are a misnomer, as they contain limits,
                # however, the naming will stay with us, so in order to avoid nested
                # "properties" (limits), we simply use VkPhysicalDeviceLimits directly here
                # for version 1.0 limits, plus, not having a better place to put them, we
                # also include VkPhysicalDeviceSparseProperties here (even though they are
                # more like features)
                limitStructNames = [ 'VkPhysicalDeviceLimits', 'VkPhysicalDeviceSparseProperties' ]
            else:
                # For all other versions use the property structures required by it
                limitStructNames = []
                xmlVersion = xml.find("./feature[@name='" + version.name + "']")
                for type in xmlVersion.findall("./require/type"):
                    name = type.get('name')
                    if name in self.structs and 'VkPhysicalDeviceProperties2' in self.structs[name].extends:
                        limitStructNames.append(name)
                # VkPhysicalDeviceVulkan11Properties is defined in Vulkan 1.2, but it actually
                # contains Vulkan 1.1 limits, so treat it as such
                if version.name == 'VK_VERSION_1_1':
                    limitStructNames.append('VkPhysicalDeviceVulkan11Properties')
                elif version.name == 'VK_VERSION_1_2':
                    limitStructNames.remove('VkPhysicalDeviceVulkan11Properties')
            # For each limit collect all property/limit structures containing them, and their aliases
            for limitStructName in limitStructNames:
                if (limitStructName in self.structs):
                    structDef = self.structs[limitStructName]
                    for memberName in structDef.members.keys():
                        if not memberName in version.limits:
                            version.limits[memberName] = VulkanLimit(memberName)
                        version.limits[memberName].structs.update(structDef.aliases)

        # Then parse properties/limits specific to extensions
        for extension in self.extensions.values():
            limitStructNames = []
            xmlExtension = xml.find("./extensions/extension[@name='" + extension.name + "']")
            for type in xmlExtension.findall("./require/type"):
                name = type.get('name')
                if name in self.structs and 'VkPhysicalDeviceProperties2' in self.structs[name].extends:
                    limitStructNames.append(name)
            # For each limit collect all property/limit structures containing them, and their aliases
            for limitStructName in limitStructNames:
                structDef = self.structs[limitStructName]
                for memberName in structDef.members.keys():
                    extension.limits[memberName] = VulkanLimit(memberName)
                    extension.limits[memberName].structs.update(structDef.aliases)
                    # For each limit we also have to check whether it's part of core so that
                    # any not strictly alias struct (i.e. the VkPhysicalDeviceVulkanXXProperties)
                    # get included as well
                    for version in self.versions.values():
                        if memberName in version.limits and version.limits[memberName].structs >= extension.limits[memberName].structs:
                            extension.limits[memberName].structs = version.limits[memberName].structs


    def parseHeaderVersion(self, xml):
        # Find the largest version number
        maxVersionNumber = self.versions[max(self.versions, key = lambda version: self.versions[version].number)].number
        self.headerVersionNumber = VulkanVersionNumber(str(maxVersionNumber))
        # Add patch from VK_HEADER_VERSION define
        for define in xml.findall("./types/type[@category='define']"):
            name = define.find('./name')
            if name != None and name.text == 'VK_HEADER_VERSION':
                self.headerVersionNumber.patch = int(name.tail.lstrip())
                return


    def applyWorkarounds(self):
        if self.headerVersionNumber.patch < 207: # vk.xml declares maxColorAttachments with 'bitmask' limittype before header 207
            self.structs['VkPhysicalDeviceLimits'].members['maxColorAttachments'].limittype = 'max'

        # TODO: We currently have to apply workarounds due to "noauto" limittypes and other bugs related to limittypes in the vk.xml
        # These can only be solved permanently if we make modifications to the registry xml itself
        self.structs['VkPhysicalDeviceLimits'].members['subPixelPrecisionBits'].limittype = 'bits'
        self.structs['VkPhysicalDeviceLimits'].members['subTexelPrecisionBits'].limittype = 'bits'
        self.structs['VkPhysicalDeviceLimits'].members['mipmapPrecisionBits'].limittype = 'bits'
        self.structs['VkPhysicalDeviceLimits'].members['viewportSubPixelBits'].limittype = 'bits'
        self.structs['VkPhysicalDeviceLimits'].members['subPixelInterpolationOffsetBits'].limittype = 'bits'
        self.structs['VkPhysicalDeviceLimits'].members['minMemoryMapAlignment'].limittype = 'min,pot'
        self.structs['VkPhysicalDeviceLimits'].members['minTexelBufferOffsetAlignment'].limittype = 'min,pot'
        self.structs['VkPhysicalDeviceLimits'].members['minUniformBufferOffsetAlignment'].limittype = 'min,pot'
        self.structs['VkPhysicalDeviceLimits'].members['minStorageBufferOffsetAlignment'].limittype = 'min,pot'
        self.structs['VkPhysicalDeviceLimits'].members['optimalBufferCopyOffsetAlignment'].limittype = 'min,pot'
        self.structs['VkPhysicalDeviceLimits'].members['optimalBufferCopyRowPitchAlignment'].limittype = 'min,pot'
        self.structs['VkPhysicalDeviceLimits'].members['nonCoherentAtomSize'].limittype = 'min,pot'
        self.structs['VkPhysicalDeviceLimits'].members['timestampPeriod'].limittype = 'noauto'
        self.structs['VkPhysicalDeviceLimits'].members['bufferImageGranularity'].limittype = 'min,mul'
        self.structs['VkPhysicalDeviceLimits'].members['pointSizeGranularity'].limittype = 'min,mul'
        self.structs['VkPhysicalDeviceLimits'].members['lineWidthGranularity'].limittype = 'min,mul'
        self.structs['VkPhysicalDeviceLimits'].members['strictLines'].limittype = 'exact'
        self.structs['VkPhysicalDeviceLimits'].members['standardSampleLocations'].limittype = 'exact'

        if 'VkPhysicalDeviceVulkan11Properties' in self.structs:
            self.structs['VkPhysicalDeviceVulkan11Properties'].members['deviceUUID'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceVulkan11Properties'].members['driverUUID'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceVulkan11Properties'].members['deviceLUID'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceVulkan11Properties'].members['deviceNodeMask'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceVulkan11Properties'].members['deviceLUIDValid'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceVulkan11Properties'].members['subgroupSize'].limittype = 'max,pot'
            self.structs['VkPhysicalDeviceVulkan11Properties'].members['pointClippingBehavior'].limittype = 'exact'
            self.structs['VkPhysicalDeviceVulkan11Properties'].members['protectedNoFault'].limittype = 'exact'

        if 'VkPhysicalDeviceVulkan12Properties' in self.structs:
            self.structs['VkPhysicalDeviceVulkan12Properties'].members['driverID'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceVulkan12Properties'].members['driverName'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceVulkan12Properties'].members['driverInfo'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceVulkan12Properties'].members['conformanceVersion'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceVulkan12Properties'].members['denormBehaviorIndependence'].limittype = 'exact'
            self.structs['VkPhysicalDeviceVulkan12Properties'].members['roundingModeIndependence'].limittype = 'exact'

        if 'VkPhysicalDeviceVulkan13Properties' in self.structs:
            self.structs['VkPhysicalDeviceVulkan13Properties'].members['storageTexelBufferOffsetAlignmentBytes'].limittype = 'min,pot'
            self.structs['VkPhysicalDeviceVulkan13Properties'].members['storageTexelBufferOffsetSingleTexelAlignment'].limittype = 'exact'
            self.structs['VkPhysicalDeviceVulkan13Properties'].members['uniformTexelBufferOffsetAlignmentBytes'].limittype = 'min,pot'
            self.structs['VkPhysicalDeviceVulkan13Properties'].members['uniformTexelBufferOffsetSingleTexelAlignment'].limittype = 'exact'
            self.structs['VkPhysicalDeviceVulkan13Properties'].members['minSubgroupSize'].limittype = 'min,pot'
            self.structs['VkPhysicalDeviceVulkan13Properties'].members['maxSubgroupSize'].limittype = 'max,pot'

        if 'VkPhysicalDeviceTexelBufferAlignmentProperties' in self.structs:
            self.structs['VkPhysicalDeviceTexelBufferAlignmentProperties'].members['storageTexelBufferOffsetAlignmentBytes'].limittype = 'min,pot'
            self.structs['VkPhysicalDeviceTexelBufferAlignmentProperties'].members['storageTexelBufferOffsetSingleTexelAlignment'].limittype = 'exact'
            self.structs['VkPhysicalDeviceTexelBufferAlignmentProperties'].members['uniformTexelBufferOffsetAlignmentBytes'].limittype = 'min,pot'
            self.structs['VkPhysicalDeviceTexelBufferAlignmentProperties'].members['uniformTexelBufferOffsetSingleTexelAlignment'].limittype = 'exact'

        if 'VkPhysicalDeviceProperties' in self.structs:
            self.structs['VkPhysicalDeviceProperties'].members['apiVersion'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceProperties'].members['driverVersion'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceProperties'].members['vendorID'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceProperties'].members['deviceID'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceProperties'].members['deviceType'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceProperties'].members['deviceName'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceProperties'].members['pipelineCacheUUID'].limittype = 'noauto'

        if 'VkPhysicalDeviceToolProperties' in self.structs:
            self.structs['VkPhysicalDeviceToolProperties'].members['name'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceToolProperties'].members['version'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceToolProperties'].members['purposes'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceToolProperties'].members['description'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceToolProperties'].members['layer'].limittype = 'noauto'

        if 'VkPhysicalDeviceSubgroupSizeControlProperties' in self.structs:
            self.structs['VkPhysicalDeviceSubgroupSizeControlProperties'].members['minSubgroupSize'].limittype = 'min,pot'
            self.structs['VkPhysicalDeviceSubgroupSizeControlProperties'].members['maxSubgroupSize'].limittype = 'max,pot'

        if 'VkPhysicalDeviceDriverProperties' in self.structs:
            self.structs['VkPhysicalDeviceDriverProperties'].members['driverID'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceDriverProperties'].members['driverName'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceDriverProperties'].members['driverInfo'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceDriverProperties'].members['conformanceVersion'].limittype = 'noauto'

        if 'VkPhysicalDeviceIDProperties' in self.structs:
            self.structs['VkPhysicalDeviceIDProperties'].members['deviceUUID'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceIDProperties'].members['driverUUID'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceIDProperties'].members['deviceLUID'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceIDProperties'].members['deviceNodeMask'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceIDProperties'].members['deviceLUIDValid'].limittype = 'noauto'

        if 'VkPhysicalDeviceSubgroupProperties' in self.structs:
            self.structs['VkPhysicalDeviceSubgroupProperties'].members['subgroupSize'].limittype = 'max,pot'

        if 'VkPhysicalDevicePointClippingProperties' in self.structs:
            self.structs['VkPhysicalDevicePointClippingProperties'].members['pointClippingBehavior'].limittype = 'exact'

        if 'VkPhysicalDeviceProtectedMemoryProperties' in self.structs:
            self.structs['VkPhysicalDeviceProtectedMemoryProperties'].members['protectedNoFault'].limittype = 'exact'

        if 'VkPhysicalDeviceFloatControlsProperties' in self.structs:
            self.structs['VkPhysicalDeviceFloatControlsProperties'].members['denormBehaviorIndependence'].limittype = 'exact'
            self.structs['VkPhysicalDeviceFloatControlsProperties'].members['roundingModeIndependence'].limittype = 'exact'

        if 'VkPhysicalDeviceTexelBufferAlignmentProperties' in self.structs:
            self.structs['VkPhysicalDeviceTexelBufferAlignmentProperties'].members['storageTexelBufferOffsetSingleTexelAlignment'].limittype = 'exact'
            self.structs['VkPhysicalDeviceTexelBufferAlignmentProperties'].members['uniformTexelBufferOffsetSingleTexelAlignment'].limittype = 'exact'

        if 'VkPhysicalDevicePortabilitySubsetPropertiesKHR' in self.structs: # BETA extension
            self.structs['VkPhysicalDevicePortabilitySubsetPropertiesKHR'].members['minVertexInputBindingStrideAlignment'].limittype = 'min,pot'

        if 'VkPhysicalDeviceFragmentShadingRatePropertiesKHR' in self.structs:
            self.structs['VkPhysicalDeviceFragmentShadingRatePropertiesKHR'].members['maxFragmentShadingRateAttachmentTexelSizeAspectRatio'].limittype = 'max,pot'
            self.structs['VkPhysicalDeviceFragmentShadingRatePropertiesKHR'].members['maxFragmentSizeAspectRatio'].limittype = 'max,pot'
            self.structs['VkPhysicalDeviceFragmentShadingRatePropertiesKHR'].members['maxFragmentShadingRateCoverageSamples'].limittype = 'max'

        if 'VkPhysicalDeviceRayTracingPipelinePropertiesKHR' in self.structs:
            self.structs['VkPhysicalDeviceRayTracingPipelinePropertiesKHR'].members['shaderGroupHandleSize'].limittype = 'exact'
            self.structs['VkPhysicalDeviceRayTracingPipelinePropertiesKHR'].members['shaderGroupBaseAlignment'].limittype = 'exact'
            self.structs['VkPhysicalDeviceRayTracingPipelinePropertiesKHR'].members['shaderGroupHandleCaptureReplaySize'].limittype = 'exact'
            self.structs['VkPhysicalDeviceRayTracingPipelinePropertiesKHR'].members['shaderGroupHandleAlignment'].limittype = 'min,pot'

        if self.headerVersionNumber.patch < 215: # vk.xml declares maxFragmentShadingRateRasterizationSamples with 'noauto' limittype before header 215
            if 'VkPhysicalDeviceFragmentShadingRatePropertiesKHR' in self.structs:
                self.structs['VkPhysicalDeviceFragmentShadingRatePropertiesKHR'].members['maxFragmentShadingRateRasterizationSamples'].limittype = 'max'

        if 'VkPhysicalDeviceConservativeRasterizationPropertiesEXT' in self.structs:
            self.structs['VkPhysicalDeviceConservativeRasterizationPropertiesEXT'].members['primitiveOverestimationSize'].limittype = 'exact'
            self.structs['VkPhysicalDeviceConservativeRasterizationPropertiesEXT'].members['extraPrimitiveOverestimationSizeGranularity'].limittype = 'min,mul'
            self.structs['VkPhysicalDeviceConservativeRasterizationPropertiesEXT'].members['conservativePointAndLineRasterization'].limittype = 'exact'
            self.structs['VkPhysicalDeviceConservativeRasterizationPropertiesEXT'].members['degenerateTrianglesRasterized'].limittype = 'exact'
            self.structs['VkPhysicalDeviceConservativeRasterizationPropertiesEXT'].members['degenerateLinesRasterized'].limittype = 'exact'

        if 'VkPhysicalDeviceLineRasterizationPropertiesEXT' in self.structs:
            self.structs['VkPhysicalDeviceLineRasterizationPropertiesEXT'].members['lineSubPixelPrecisionBits'].limittype = 'bits'

        if self.headerVersionNumber.patch < 213:
            if 'VkPhysicalDeviceTransformFeedbackPropertiesEXT' in self.structs:
                self.structs['VkPhysicalDeviceTransformFeedbackPropertiesEXT'].members['maxTransformFeedbackBufferDataStride'].limittype = 'max'

        if 'VkPhysicalDeviceExternalMemoryHostPropertiesEXT' in self.structs:
            self.structs['VkPhysicalDeviceExternalMemoryHostPropertiesEXT'].members['minImportedHostPointerAlignment'].limittype = 'min,pot'

        if 'VkPhysicalDevicePCIBusInfoPropertiesEXT' in self.structs:
            self.structs['VkPhysicalDevicePCIBusInfoPropertiesEXT'].members['pciDomain'].limittype = 'noauto'
            self.structs['VkPhysicalDevicePCIBusInfoPropertiesEXT'].members['pciBus'].limittype = 'noauto'
            self.structs['VkPhysicalDevicePCIBusInfoPropertiesEXT'].members['pciDevice'].limittype = 'noauto'
            self.structs['VkPhysicalDevicePCIBusInfoPropertiesEXT'].members['pciFunction'].limittype = 'noauto'

        if 'VkPhysicalDeviceDrmPropertiesEXT' in self.structs:
            self.structs['VkPhysicalDeviceDrmPropertiesEXT'].members['hasPrimary'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceDrmPropertiesEXT'].members['hasRender'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceDrmPropertiesEXT'].members['primaryMajor'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceDrmPropertiesEXT'].members['primaryMinor'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceDrmPropertiesEXT'].members['renderMajor'].limittype = 'noauto'
            self.structs['VkPhysicalDeviceDrmPropertiesEXT'].members['renderMinor'].limittype = 'noauto'

        if 'VkPhysicalDeviceFragmentDensityMap2PropertiesEXT' in self.structs:
            self.structs['VkPhysicalDeviceFragmentDensityMap2PropertiesEXT'].members['subsampledLoads'].limittype = 'exact'
            self.structs['VkPhysicalDeviceFragmentDensityMap2PropertiesEXT'].members['subsampledCoarseReconstructionEarlyAccess'].limittype = 'exact'

        if 'VkPhysicalDeviceSampleLocationsPropertiesEXT' in self.structs:
            self.structs['VkPhysicalDeviceSampleLocationsPropertiesEXT'].members['sampleLocationSubPixelBits'].limittype = 'bits'

        if 'VkPhysicalDeviceRobustness2PropertiesEXT' in self.structs:
            self.structs['VkPhysicalDeviceRobustness2PropertiesEXT'].members['robustStorageBufferAccessSizeAlignment'].limittype = 'min,pot'
            self.structs['VkPhysicalDeviceRobustness2PropertiesEXT'].members['robustUniformBufferAccessSizeAlignment'].limittype = 'min,pot'

        if 'VkPhysicalDeviceShaderCorePropertiesAMD' in self.structs:
            self.structs['VkPhysicalDeviceShaderCorePropertiesAMD'].members['shaderEngineCount'].limittype = 'exact'
            self.structs['VkPhysicalDeviceShaderCorePropertiesAMD'].members['shaderArraysPerEngineCount'].limittype = 'exact'
            self.structs['VkPhysicalDeviceShaderCorePropertiesAMD'].members['computeUnitsPerShaderArray'].limittype = 'exact'
            self.structs['VkPhysicalDeviceShaderCorePropertiesAMD'].members['simdPerComputeUnit'].limittype = 'exact'
            self.structs['VkPhysicalDeviceShaderCorePropertiesAMD'].members['wavefrontsPerSimd'].limittype = 'exact'
            self.structs['VkPhysicalDeviceShaderCorePropertiesAMD'].members['wavefrontSize'].limittype = 'max'
            self.structs['VkPhysicalDeviceShaderCorePropertiesAMD'].members['sgprsPerSimd'].limittype = 'exact'
            self.structs['VkPhysicalDeviceShaderCorePropertiesAMD'].members['sgprAllocationGranularity'].limittype = 'min,mul'
            self.structs['VkPhysicalDeviceShaderCorePropertiesAMD'].members['vgprsPerSimd'].limittype = 'exact'
            self.structs['VkPhysicalDeviceShaderCorePropertiesAMD'].members['vgprAllocationGranularity'].limittype = 'min,mul'

        if 'VkPhysicalDeviceSubpassShadingPropertiesHUAWEI' in self.structs:
            self.structs['VkPhysicalDeviceSubpassShadingPropertiesHUAWEI'].members['maxSubpassShadingWorkgroupSizeAspectRatio'].limittype = 'max,pot'

        if 'VkPhysicalDeviceRayTracingPropertiesNV' in self.structs:
            self.structs['VkPhysicalDeviceRayTracingPropertiesNV'].members['shaderGroupHandleSize'].limittype = 'exact'
            self.structs['VkPhysicalDeviceRayTracingPropertiesNV'].members['shaderGroupBaseAlignment'].limittype = 'exact'

        if 'VkPhysicalDeviceShadingRateImagePropertiesNV' in self.structs:
            self.structs['VkPhysicalDeviceShadingRateImagePropertiesNV'].members['shadingRateTexelSize'].limittype = 'exact'

        if 'VkPhysicalDeviceMeshShaderPropertiesNV' in self.structs:
            self.structs['VkPhysicalDeviceMeshShaderPropertiesNV'].members['meshOutputPerVertexGranularity'].limittype = 'min,mul'
            self.structs['VkPhysicalDeviceMeshShaderPropertiesNV'].members['meshOutputPerPrimitiveGranularity'].limittype = 'min,mul'

        if 'VkPhysicalDevicePipelineRobustnessPropertiesEXT' in self.structs:
            self.structs['VkPhysicalDevicePipelineRobustnessPropertiesEXT'].members['defaultRobustnessStorageBuffers'].limittype = 'exact'
            self.structs['VkPhysicalDevicePipelineRobustnessPropertiesEXT'].members['defaultRobustnessUniformBuffers'].limittype = 'exact'
            self.structs['VkPhysicalDevicePipelineRobustnessPropertiesEXT'].members['defaultRobustnessVertexInputs'].limittype = 'exact'
            self.structs['VkPhysicalDevicePipelineRobustnessPropertiesEXT'].members['defaultRobustnessImages'].limittype = 'exact'

        if self.headerVersionNumber.patch < 213:
            if 'VkPhysicalDeviceDeviceGeneratedCommandsPropertiesNV' in self.structs:
                self.structs['VkPhysicalDeviceDeviceGeneratedCommandsPropertiesNV'].members['minSequencesCountBufferOffsetAlignment'].limittype = 'min'
                self.structs['VkPhysicalDeviceDeviceGeneratedCommandsPropertiesNV'].members['minSequencesIndexBufferOffsetAlignment'].limittype = 'min'
                self.structs['VkPhysicalDeviceDeviceGeneratedCommandsPropertiesNV'].members['minIndirectCommandsBufferOffsetAlignment'].limittype = 'min'

        if 'VkPhysicalDeviceFragmentDensityMapOffsetPropertiesQCOM' in self.structs:
            self.structs['VkPhysicalDeviceFragmentDensityMapOffsetPropertiesQCOM'].members['fragmentDensityOffsetGranularity'].limittype = 'min,mul'

        # TODO: The registry xml is also missing limittype definitions for format and queue family properties
        # For now we just add the important ones, this needs a larger overhaul in the vk.xml
        self.structs['VkFormatProperties'].members['linearTilingFeatures'].limittype = 'bitmask'
        self.structs['VkFormatProperties'].members['optimalTilingFeatures'].limittype = 'bitmask'
        self.structs['VkFormatProperties'].members['bufferFeatures'].limittype = 'bitmask'
        if 'VkFormatProperties3' in self.structs:
            self.structs['VkFormatProperties3'].members['linearTilingFeatures'].limittype = 'bitmask'
            self.structs['VkFormatProperties3'].members['optimalTilingFeatures'].limittype = 'bitmask'
            self.structs['VkFormatProperties3'].members['bufferFeatures'].limittype = 'bitmask'

        self.structs['VkQueueFamilyProperties'].members['queueFlags'].limittype = 'bitmask'
        self.structs['VkQueueFamilyProperties'].members['queueCount'].limittype = 'max'
        self.structs['VkQueueFamilyProperties'].members['timestampValidBits'].limittype = 'bits'
        self.structs['VkQueueFamilyProperties'].members['minImageTransferGranularity'].limittype = 'min,mul'

        self.structs['VkSparseImageFormatProperties'].members['aspectMask'].limittype = 'bitmask'
        self.structs['VkSparseImageFormatProperties'].members['imageGranularity'].limittype = 'min,mul'
        self.structs['VkSparseImageFormatProperties'].members['flags'].limittype = 'bitmask'

        # TODO: The registry xml contains some return structures that contain count + pointers to arrays
        # While the script itself is prepared to drop those, as they are ill-formed, as return structures
        # should never contain such pointers, some of the structures (e.g. 'VkVideoProfilesKHR') actually
        # doesn't even have the proper 'len' attribute to be able to detect the dynamic array
        # Hence here we simply remove such "disallow-listed" structs so that they don't get in the way
        self.structs.pop('VkDrmFormatModifierPropertiesListEXT', None)
        self.structs.pop('VkDrmFormatModifierPropertiesList2EXT', None)
        self.structs.pop('VkVideoProfilesKHR', None)


    def getExtensionPromotedToVersion(self, extensionName):
        promotedTo = self.extensions[extensionName].promotedTo
        version = None
        while promotedTo != None:
            if promotedTo in self.extensions:
                # Functionality was promoted to another extension, continue with that
                promotedTo = self.extensions[promotedTo].promotedTo
            elif promotedTo in self.versions:
                # Found extension in a core API version, we're done
                version = self.versions[promotedTo]
                break
        return version


    def getChainableStructDef(self, name, extends):
        structDef = self.structs.get(name)
        if structDef == None:
            Log.f("Structure '{0}' does not exist".format(name))
        if structDef.sType == None:
            Log.f("Structure '{0}' is not chainable".format(name))
        if not extends in structDef.extends + [ name ]:
            Log.f("Structure '{0}' does not extend '{1}'".format(name, extends))
        return structDef


    def evalArraySize(self, arraySize):
        if isinstance(arraySize, str):
            if arraySize in self.constants:
                return int(self.constants[arraySize])
            else:
                Log.f("Invalid array size '{0}'".format(arraySize))
        else:
            return arraySize

    def getNonAliasTypeName(self, alias, types):
        typeDef = types[alias]
        if typeDef.isAlias:
            for alias in typeDef.aliases:
                if not types[alias].isAlias:
                    return alias
        else:
            return alias


class VulkanProfileCapabilities():
    def __init__(self, registry, data, caps):
        self.extensions = dict()
        self.instanceExtensions = dict()
        self.deviceExtensions = dict()
        self.features = dict()
        self.properties = dict()
        self.formats = dict()
        self.queueFamiliesProperties = []
        for capName in data['capabilities']:
            # When we have multiple possible capabilities blocks, we load them all but effectively the API library can't effectively implement this behavior.
            if type(capName).__name__ == 'list':
                for capNameCase in capName:
                    self.mergeCaps(registry, caps[capNameElement])
            elif capName in caps:
                self.mergeCaps(registry, caps[capName])
            else:
                Log.f("Capability '{0}' needed by profile '{1}' is missing".format(capName, data['name']))

    def mergeCaps(self, registry, caps):
        self.mergeProfileExtensions(registry, caps)
        self.mergeProfileFeatures(caps)
        self.mergeProfileProperties(caps)
        self.mergeProfileFormats(caps)
        self.mergeProfileQueueFamiliesProperties(caps)


    def mergeProfileCapData(self, dst, src):
        if type(src) != type(dst):
            Log.f("Data type confict during profile capability data merge (src is '{0}', dst is '{1}')".format(type(src), type(dst)))
        elif type(src) == dict:
            for key, val in src.items():
                if type(val) == dict:
                    if not key in dst:
                        dst[key] = dict()
                    self.mergeProfileCapData(dst[key], val)

                elif type(val) == list:
                    if not key in dst:
                        dst[key] = []
                    dst[key].extend(val)

                else:
                    if key in dst and type(dst[key]) != type(val):
                        Log.f("Data type confict during profile capability data merge (src is '{0}', dst is '{1}')".format(type(val), type(dst[key])))
                    dst[key] = val
        else:
            Log.f("Unexpected data type during profile capability data merge (src is '{0}', dst is '{1}')".format(type(src), type(dst)))


    def mergeProfileExtensions(self, registry, data):
        if data.get('extensions') != None:
            for extName, specVer in data['extensions'].items():
                extInfo = registry.extensions.get(extName)
                if extInfo != None:
                    self.extensions[extName] = specVer
                    if extInfo.type == 'instance':
                        self.instanceExtensions[extName] = specVer
                    elif extInfo.type == 'device':
                        self.deviceExtensions[extName] = specVer
                    else:
                        Log.f("Extension '{0}' has invalid type '{1}'".format(extName, extInfo.type))
                else:
                    Log.f("Extension '{0}' does not exist".format(extName))


    def mergeProfileFeatures(self, data):
        if data.get('features') != None:
            self.mergeProfileCapData(self.features, data['features'])


    def mergeProfileProperties(self, data):
        if data.get('properties') != None:
            self.mergeProfileCapData(self.properties, data['properties'])


    def mergeProfileFormats(self, data):
        if data.get('formats') != None:
            self.mergeProfileCapData(self.formats, data['formats'])


    def mergeProfileQueueFamiliesProperties(self, data):
        if data.get('queueFamiliesProperties') != None:
            self.queueFamiliesProperties.extend(data['queueFamiliesProperties'])


class VulkanProfileStructs():
    def __init__(self, registry, caps):
        # Feature struct types
        self.feature = []
        for name in caps.features:
            if name in [ 'VkPhysicalDeviceFeatures', 'VkPhysicalDeviceFeatures2KHR' ]:
                # Special case, add both as VkPhysicalDeviceFeatures2KHR
                self.feature.append(registry.structs['VkPhysicalDeviceFeatures2KHR'])
            else:
                self.feature.append(registry.getChainableStructDef(name, 'VkPhysicalDeviceFeatures2'))
        self.eliminateAliases(self.feature)

        # Property struct types
        self.property = []
        for name in caps.properties:
            if name in [ 'VkPhysicalDeviceProperties', 'VkPhysicalDeviceProperties2KHR' ]:
                # Special case, add both as VkPhysicalDeviceProperties2KHR
                self.property.append(registry.structs['VkPhysicalDeviceProperties2KHR'])
            else:
                self.property.append(registry.getChainableStructDef(name, 'VkPhysicalDeviceProperties2'))
        self.eliminateAliases(self.property)

        # Queue family struct types
        self.queueFamily = []
        queueFamilyStructs = dict()
        for queueFamilyProps in caps.queueFamiliesProperties:
            queueFamilyStructs.update(queueFamilyProps)
        for name in queueFamilyStructs:
            if name in [ 'VkQueueFamilyProperties', 'VkQueueFamilyProperties2KHR' ]:
                # Special case, add both as VkQueueFamilyProperties2KHR
                self.queueFamily.append(registry.structs['VkQueueFamilyProperties2KHR'])
            else:
                self.queueFamily.append(registry.getChainableStructDef(name, 'VkQueueFamilyProperties2'))
        self.eliminateAliases(self.queueFamily)

        # Format struct types
        self.format = []
        formatStructs = dict()
        for formatProps in caps.formats.values():
            formatStructs.update(formatProps)
        for name in formatStructs:
            if name in [ 'VkFormatProperties', 'VkFormatProperties2KHR', 'VkFormatProperties3KHR' ]:
                # Special case, add all as VkFormatProperties2KHR and VkFormatProperties3KHR
                self.format.append(registry.structs['VkFormatProperties2KHR'])
                if 'VkFormatProperties3KHR' in registry.structs:
                    self.format.append(registry.structs['VkFormatProperties3KHR'])
            else:
                self.format.append(registry.getChainableStructDef(name, 'VkFormatProperties2'))
        self.eliminateAliases(self.format)


    def eliminateAliases(self, structs):
        structNames = []
        duplicates = []
        # Collect duplicates
        for structDef in structs:
            if structDef.name in structNames:
                duplicates.append(structDef)
            structNames.append(structDef.aliases)
        # Remove duplicates
        for duplicate in duplicates:
            structs.remove(duplicate)


class VulkanProfile():
    def __init__(self, registry, name, data, caps):
        self.registry = registry
        self.name = name
        self.label = data['label']
        self.description = data['description']
        self.version = data['version']
        self.apiVersion = data['api-version']
        self.apiVersionNumber = VulkanVersionNumber(self.apiVersion)
        self.fallback = data.get('fallback')
        self.versionRequirements = []
        self.extensionRequirements = []
        self.capabilities = VulkanProfileCapabilities(registry, data, caps)
        self.structs = VulkanProfileStructs(registry, self.capabilities)
        self.collectCompileTimeRequirements()
        self.validate()


    def collectCompileTimeRequirements(self):
        # Add API version to the list of requirements
        versionName = self.apiVersionNumber.define
        if versionName in self.registry.versions:
            self.versionRequirements.append(versionName)
        else:
            Log.f("No version '{0}' found in registry required by profile '{1}'".format(str(self.apiVersionNumber), self.name))

        # Add any required extension to the list of requirements
        for extName in self.capabilities.extensions:
            if extName in self.registry.extensions:
                self.extensionRequirements.append(extName)
            else:
                Log.f("Extension '{0}' required by profile '{1}' does not exist".format(extName, self.name))


    def validate(self):
        self.validateStructDependencies()


    def validateStructDependencies(self):
        for feature in self.capabilities.features:
            self.validateStructDependency(feature)

        for prop in self.capabilities.properties:
            self.validateStructDependency(prop)

        for queueFamilyData in self.capabilities.queueFamiliesProperties:
            for queueFamilyProp in queueFamilyData:
                self.validateStructDependency(queueFamilyProp)


    def validateStructDependency(self, structName):
        if structName in self.registry.structs:
            structDef = self.registry.structs[structName]
            depFound = False

            # Check if the required API version defines this struct
            if structDef.definedByVersion != None and structDef.definedByVersion <= self.apiVersionNumber:
                depFound = True

            # Check if any required extension defines this struct
            for definedByExtension in structDef.definedByExtensions:
                if definedByExtension in self.capabilities.extensions:
                    depFound = True
                    break

            if not depFound:
                Log.f("Unexpected required struct '{0}' in profile '{1}'".format(structName, self.name))
        else:
            Log.f("Struct '{0}' in profile '{1}' does not exist in the registry".format(structName, self.name))


    def generatePrivateImpl(self, debugMessages):
        uname = self.name.upper()
        gen = '\n'
        gen += ('#ifdef {0}\n'
                'namespace {1} {{\n').format(self.name, uname)
        gen += self.gen_extensionData('instance')
        gen += self.gen_extensionData('device')
        gen += self.gen_fallbackData()
        gen += self.gen_structTypeData()
        gen += self.gen_structDesc(debugMessages)
        gen += ('\n'
                '}} // namespace {0}\n'
                '#endif\n').format(uname)
        return gen

    def gen_extensionData(self, type):
        foundExt = False
        gen = '\n'
        gen += 'static const VkExtensionProperties {0}Extensions[] = {{\n'.format(type)
        for extName, specVer in sorted(self.capabilities.extensions.items()):
            extInfo = self.registry.extensions[extName]
            if extInfo.type == type:
                gen += '    VkExtensionProperties{{ {0}_EXTENSION_NAME, {1} }},\n'.format(extInfo.upperCaseName, specVer)
                foundExt = True
        gen += '};\n'
        return gen if foundExt else ''


    def gen_fallbackData(self):
        gen = ''
        if self.fallback:
            gen += ('\n'
                    'static const VpProfileProperties fallbacks[] = {\n')
            for fallback in self.fallback:
                gen += '    {{ {0}_NAME, {0}_SPEC_VERSION }},\n'.format(fallback.upper())
            gen += '};\n'
        return gen


    def gen_structTypeData(self, structDefs = None, name = None):
        gen = ''
        if structDefs == None:
            gen += self.gen_structTypeData(self.structs.feature, 'feature')
            gen += self.gen_structTypeData(self.structs.property, 'property')
            gen += self.gen_structTypeData(self.structs.queueFamily, 'queueFamily')
            gen += self.gen_structTypeData(self.structs.format, 'format')
        else:
            if structDefs:
                gen += ('\n'
                        'static const VkStructureType {0}StructTypes[] = {{\n').format(name)
                for structDef in structDefs:
                    gen += '    {0},\n'.format(structDef.sType)
                gen += '};\n'
        return gen


    def gen_listValue(self, values, isEnum = True):
        gen = ''
        if isEnum:
            gen += '('
        else:
            gen += '{ '

        separator = ''
        if values != None and len(values) > 0:
            for value in values:
                gen += separator + str(value)
                if isEnum:
                    separator = ' | '
                else:
                    separator = ', '
        elif isEnum:
            gen += '0'

        if isEnum:
            gen += ')'
        else:
            gen += ' }'
        return gen


    def gen_structFill(self, fmt, structDef, var, values):
        gen = ''
        for member, value in sorted(values.items()):
            if member in structDef.members:
                if type(value) == dict:
                    # Nested structure
                    memberDef = self.registry.structs.get(structDef.members[member].type)
                    if memberDef != None:
                        gen += self.gen_structFill(fmt, memberDef, var + member + '.', value)
                    else:
                        Log.f("Member '{0}' in structure '{1}' is not a struct".format(member, structDef.name))

                elif type(value) == list:
                    # Some sort of list (enums or integer/float list for structure initialization)
                    if len(value) == 0:
                        # If list is empty then ignore
                        continue
                    if structDef.members[member].isArray:
                        if not isinstance(self.registry.evalArraySize(structDef.members[member].arraySize), int):
                            Log.f("Unsupported array member '{0}' in structure '{1}'".format(member, structDef.name) +
                                  "(currently only 1D non-dynamic arrays are supported in this context)")
                        # If it's an array we have to generate per-element assignment code
                        for i, v in enumerate(value):
                            if type(v) == float:
                                if structDef.members[member].type == 'double':
                                    gen += fmt.format('{0}{1}[{2}] = {3}'.format(var, member, i, v))
                                else:
                                    gen += fmt.format('{0}{1}[{2}] = {3}f'.format(var, member, i, v))
                            else:
                                gen += fmt.format('{0}{1}[{2}] = {3}'.format(var, member, i, v))
                    else:
                        # For enums and struct initialization, most of the code can be shared
                        isEnum = isinstance(value[0], str)
                        if isEnum:
                            # For enums we only add bits
                            genAssign = '{0}{1} = '.format(var, member)
                        else:
                            genAssign = '{0}{1} = '.format(var, member)
                        genAssign += '{0}'.format(self.gen_listValue(value, isEnum))
                        gen += fmt.format(genAssign)
                elif type(value) == float:
                    if structDef.members[member].type == 'double':
                        gen += fmt.format('{0}{1} = {2}'.format(var, member, value))
                    else:
                        gen += fmt.format('{0}{1} = {2}f'.format(var, member, value))
                elif type(value) == bool:
                    # Boolean
                    gen += fmt.format('{0}{1} = {2}'.format(var, member, 'VK_TRUE' if value else 'VK_FALSE'))

                else:
                    # Everything else
                    gen += fmt.format('{0}{1} = {2}'.format(var, member, value))
            else:
                Log.f("No member '{0}' in structure '{1}'".format(member, structDef.name))
        return gen


    def gen_structCompare(self, fmt, structDef, var, values, parentLimittype = None):
        gen = ''
        for member, value in sorted(values.items()):
            if member in structDef.members:
                limittype = structDef.members[member].limittype
                membertype = structDef.members[member].type
                if limittype == None:
                    # Use parent's limit type
                    limittype = parentLimittype

                if limittype == 'bitmask' and type == 'VkBool32':
                    # Compare everything else with equality
                    comparePredFmt = '{0} == {1}'
                elif limittype == 'bitmask':
                    # Compare bitmask by checking if device value contains every bit of profile value
                    comparePredFmt = 'vpCheckFlags({0}, {1})'
                elif limittype == 'bits':
                    # Compare max limit by checking if device value is greater than or equal to profile value
                    comparePredFmt = '{0} >= {1}'
                elif limittype == 'max':
                    # Compare max limit by checking if device value is greater than or equal to profile value
                    comparePredFmt = '{0} >= {1}'
                elif limittype == 'max,pot' or limittype == 'pot,max':
                    # Compare max limit by checking if device value is greater than or equal to profile value
                    if (membertype == 'float' or membertype == 'double'):
                        comparePredFmt = [ '{0} >= {1}' ]
                    else:
                        comparePredFmt = [ '{0} >= {1}', '({0} & ({0} - 1)) == 0' ]
                elif limittype == 'bits':
                    # Behaves like max, but smaller values are allowed
                    comparePredFmt = '{0} >= {1}'
                elif limittype == 'min':
                    # Compare min limit by checking if device value is less than or equal to profile value
                    comparePredFmt = '{0} <= {1}'
                elif limittype == 'pot':
                    if (membertype == 'float' or membertype == 'double'):
                        comparePredFmt = [ 'isPowerOfTwo({0})' ]
                    else:
                        comparePredFmt = [ '({0} & ({0} - 1)) == 0' ]
                elif limittype == 'min,pot' or limittype == 'pot,min':
                    # Compare min limit by checking if device value is less than or equal to profile value and if the value is a power of two
                    if (membertype == 'float' or membertype == 'double'):
                        comparePredFmt = [ '{0} <= {1}', 'isPowerOfTwo({0})' ]
                    else:
                        comparePredFmt = [ '{0} <= {1}', '({0} & ({0} - 1)) == 0' ]
                elif limittype == 'min,mul' or limittype == 'mul,min':
                    # Compare min limit by checking if device value is less than or equal to profile value and a multiple of profile value
                    if (membertype == 'float' or membertype == 'double'):
                        comparePredFmt = [ '{0} <= {1}', 'isMultiple({1}, {0})' ]
                    else:
                        comparePredFmt = [ '{0} <= {1}', '({1} % {0}) == 0' ]
                elif limittype == 'range':
                    # Compare range limit by checking if device range is larger than or equal to profile range
                    comparePredFmt = [ '{0} <= {1}', '{0} >= {1}' ]
                elif limittype == 'exact' or limittype == 'struct':
                    # Compare everything else with equality
                    comparePredFmt = '{0} == {1}'
                elif limittype is None or limittype == 'noauto':
                    comparePredFmt = '{0} == {1}'
                else:
                    Log.f("Unsupported limittype '{0}' in member '{1}' of structure '{2}'".format(limittype, member, structDef.name))

                if type(value) == dict:
                    # Nested structure
                    memberDef = self.registry.structs.get(structDef.members[member].type)
                    if memberDef != None:
                        gen += self.gen_structCompare(fmt, memberDef, var + member + '.', value, limittype)
                    else:
                        Log.f("Member '{0}' in structure '{1}' is not a struct".format(member, structDef.name))

                elif type(value) == list:
                    # Some sort of list (enums or integer/float list for structure initialization)
                    if len(value) == 0:
                        # If list is empty then ignore
                        continue
                    if structDef.members[member].isArray:
                        if not isinstance(self.registry.evalArraySize(structDef.members[member].arraySize), int):
                            Log.f("Unsupported array member '{0}' in structure '{1}'".format(member, structDef.name) +
                                  "(currently only 1D non-dynamic arrays are supported in this context)")
                        # If it's an array we have to generate per-element comparison code
                        for i in range(len(value)):
                            if limittype == 'range':
                                gen += fmt.format(comparePredFmt[i].format('{0}{1}[{2}]'.format(var, member, i), value[i]))
                            else:
                                gen += fmt.format(comparePredFmt.format('{0}{1}[{2}]'.format(var, member, i), value[i]))
                    else:
                        # Enum flags and basic structs can be compared directly
                        isEnum = isinstance(value[0], str)
                        gen += fmt.format(comparePredFmt.format('{0}{1}'.format(var, member), self.gen_listValue(value, isEnum)))

                elif type(value) == bool:
                    # Boolean
                    gen += fmt.format(comparePredFmt.format('{0}{1}'.format(var, member), 'VK_TRUE' if value else 'VK_FALSE'))

                else:
                    # Everything else
                    if type(comparePredFmt) == list:
                        for i in range(len(comparePredFmt)):
                            gen += fmt.format(comparePredFmt[i].format('{0}{1}'.format(var, member), value))
                    elif comparePredFmt is not None:
                        gen += fmt.format(comparePredFmt.format('{0}{1}'.format(var, member), value))
            else:
                Log.f("No member '{0}' in structure '{1}'".format(member, structDef.name))
        return gen


    def gen_structFunc(self, structDefs, caps, func, fmt, debugMessages = False):
        gen = ''

        hasData = False

        gen += ('            switch (p->sType) {\n')

        for structDef in structDefs:
            paramList = []

            # Fill VkPhysicalDeviceFeatures into VkPhysicalDeviceFeatures2[KHR]
            if structDef.name in ['VkPhysicalDeviceFeatures2', 'VkPhysicalDeviceFeatures2KHR']:
                innerCap = caps.get('VkPhysicalDeviceFeatures')
                if innerCap:
                    paramList.append((self.registry.structs['VkPhysicalDeviceFeatures'], '->features.', innerCap))

            # Fill VkPhysicalDeviceProperties into VkPhysicalDeviceProperties2[KHR]
            if structDef.name in ['VkPhysicalDeviceProperties2', 'VkPhysicalDeviceProperties2KHR']:
                innerCap = caps.get('VkPhysicalDeviceProperties')
                if innerCap:
                    paramList.append((self.registry.structs['VkPhysicalDeviceProperties'], '->properties.', innerCap))

            # Fill VkQueueFamilyProperties into VkQueueFamilyProperties2[KHR]
            if structDef.name in ['VkQueueFamilyProperties2', 'VkQueueFamilyProperties2KHR']:
                innerCap = caps.get('VkQueueFamilyProperties')
                if innerCap:
                    paramList.append((self.registry.structs['VkQueueFamilyProperties'], '->queueFamilyProperties.', innerCap))

            # Fill VkFormatProperties into VkFormatProperties2[KHR]
            if structDef.name in ['VkFormatProperties2', 'VkFormatProperties2KHR']:
                innerCap = caps.get('VkFormatProperties')
                if innerCap:
                    paramList.append((self.registry.structs['VkFormatProperties'], '->formatProperties.', innerCap))

            # Fill all other structures directly
            if structDef.name in caps:
                paramList.append((structDef, '->', caps[structDef.name]))

            # Use variable names in the debug version of the library that can be later prettified
            if debugMessages:
                varName = 'prettify_' + structDef.name
            else:
                varName = 's'

            if paramList:
                gen += '                case {0}: {{\n'.format(structDef.sType)
                gen += '                    {0}* {1} = static_cast<{0}*>(static_cast<void*>(p));\n'.format(structDef.name, varName)
                for params in paramList:
                    genAssign = func('                    ' + fmt, params[0], varName + params[1], params[2])
                    if genAssign != '':
                        hasData = True
                        gen += genAssign
                gen += '                } break;\n'

        gen += ('                default: break;\n'
                '            }\n')
        return gen if hasData else ''


    def gen_structChainerFunc(self, structDefs, baseStruct):
        gen = '    [](VkBaseOutStructure* p, void* pUser, PFN_vpStructChainerCb pfnCb) {\n'
        if structDefs:
            pNext = 'nullptr'
            for structDef in structDefs:
                if structDef.name != baseStruct:
                    varName = structDef.name[2].lower() + structDef.name[3:]
                    gen += '        {0} {1}{{ {2}, {3} }};\n'.format(structDef.name, varName, structDef.sType, pNext)
                    pNext = '&' + varName
            gen += '        p->pNext = static_cast<VkBaseOutStructure*>(static_cast<void*>({0}));\n'.format(pNext)

        gen += ('        pfnCb(p, pUser);\n'
                '    },\n')
        return gen



    def gen_structDesc(self, debugMessages):
        gen = ''

        fillFmt = '{0};\n'
        cmpFmt = 'ret = ret && ({0});\n'

        # Feature descriptor
        if debugMessages:
            cmpFmtFeatures = 'ret = ret && ({0}); VP_DEBUG_COND_MSG(!({0}), "Unsupported feature condition: {0}");\n'
        else:
            cmpFmtFeatures = cmpFmt

        gen += ('\n'
                'static const VpFeatureDesc featureDesc = {\n'
                '    [](VkBaseOutStructure* p) {\n')
        gen += self.gen_structFunc(self.structs.feature, self.capabilities.features, self.gen_structFill, fillFmt)
        gen += ('    },\n'
                '    [](VkBaseOutStructure* p) -> bool {\n'
                '        bool ret = true;\n')
        gen += self.gen_structFunc(self.structs.feature, self.capabilities.features, self.gen_structCompare, cmpFmtFeatures, debugMessages)
        gen += ('        return ret;\n'
                '    }\n'
                '};\n')

        # Property descriptor
        if debugMessages:
            cmpFmtProperties = 'ret = ret && ({0}); VP_DEBUG_COND_MSG(!({0}), "Unsupported properties condition: {0}");\n'
        else:
            cmpFmtProperties = cmpFmt

        gen += ('\n'
                'static const VpPropertyDesc propertyDesc = {\n'
                '    [](VkBaseOutStructure* p) {\n')
        gen += self.gen_structFunc(self.structs.property, self.capabilities.properties, self.gen_structFill, fillFmt)
        gen += ('    },\n'
                '    [](VkBaseOutStructure* p) -> bool {\n'
                '        bool ret = true;\n')
        gen += self.gen_structFunc(self.structs.property, self.capabilities.properties, self.gen_structCompare, cmpFmtProperties, debugMessages)
        gen += ('        return ret;\n'
                '    }\n'
                '};\n')

        # Queue family descriptor
        if self.structs.queueFamily:
            gen += ('\n'
                    'static const VpQueueFamilyDesc queueFamilyDesc[] = {\n')
            for queueFamilyCaps in self.capabilities.queueFamiliesProperties:
                gen += ('    {\n'
                        '        [](VkBaseOutStructure* p) {\n')
                gen += self.gen_structFunc(self.structs.queueFamily, queueFamilyCaps, self.gen_structFill, fillFmt)
                gen += ('        },\n'
                        '        [](VkBaseOutStructure* p) -> bool {\n'
                        '            bool ret = true;\n')
                gen += self.gen_structFunc(self.structs.queueFamily, queueFamilyCaps, self.gen_structCompare, cmpFmt)
                gen += ('            return ret;\n'
                        '        }\n'
                        '    },\n')
            gen += ('};\n')

        # Format descriptor
        if self.structs.format:
            gen += ('\n'
                    'static const VpFormatDesc formatDesc[] = {\n')
            for formatName, formatCaps in sorted(self.capabilities.formats.items()):
                if debugMessages:
                    cmpFmtFormat = 'ret = ret && ({0}); VP_DEBUG_COND_MSG(!({0}), "Unsupported format condition for ' + formatName + ': {0}");\n'
                else:
                    cmpFmtFormat = cmpFmt

                gen += ('    {{\n'
                        '        {0},\n'
                        '        [](VkBaseOutStructure* p) {{\n').format(formatName)
                gen += self.gen_structFunc(self.structs.format, formatCaps, self.gen_structFill, fillFmt)
                gen += ('        },\n'
                        '        [](VkBaseOutStructure* p) -> bool {\n'
                        '            bool ret = true;\n')
                gen += self.gen_structFunc(self.structs.format, formatCaps, self.gen_structCompare, cmpFmtFormat, debugMessages)
                gen += ('            return ret;\n'
                        '        }\n'
                        '    },\n')
            gen += '};\n'

        # Structure chaining descriptors
        gen += ('\n'
                'static const VpStructChainerDesc chainerDesc = {\n')
        gen += self.gen_structChainerFunc(self.structs.feature, 'VkPhysicalDeviceFeatures2KHR')
        gen += self.gen_structChainerFunc(self.structs.property, 'VkPhysicalDeviceProperties2KHR')
        gen += self.gen_structChainerFunc(self.structs.queueFamily, 'VkQueueFamilyProperties2KHR')
        gen += self.gen_structChainerFunc(self.structs.format, 'VkFormatProperties2KHR')
        gen += '};\n'

        # If debug messages are needed do further prettifying (warning: obscure regular expressions follow)
        if debugMessages:
            # Prettify structure references in non-bitmask comparisons
            gen = re.sub(r"(VP_DEBUG_COND_MSG\([^,]+[^:]+: )prettify_Vk([^\-]+)\->([^\)]+\))", r"\1Vk\2::\3", gen)
            # Prettify bitmask comparisons
            gen = re.sub(r"(VP_DEBUG_COND_MSG\([^,]+[^:]+: )vpCheckFlags\(prettify_Vk([^\-]+)\->([^,]+), ([^\)]+)\)", r"\1Vk\2::\3 contains \4", gen)

        return gen


class VulkanProfiles():
    def loadFromDir(registry, profilesDir, validate, schema):
        profiles = dict()
        dirAbsPath = os.path.abspath(profilesDir)
        filenames = os.listdir(dirAbsPath)
        for filename in filenames:
            fileAbsPath = os.path.join(dirAbsPath, filename)
            if os.path.isfile(fileAbsPath) and os.path.splitext(filename)[-1] == '.json':
                Log.i("Loading profile file: '{0}'".format(filename))
                with open(fileAbsPath, 'r') as f:
                    jsonData = json.load(f)
                    if validate:
                        Log.i("Validating profile file: '{0}'".format(filename))
                        # jsonschema.validate(jsonData, schema)
                    VulkanProfiles.parseProfiles(registry, profiles, jsonData['profiles'], jsonData['capabilities'])
        return profiles


    def parseProfiles(registry, profiles, json, caps):
        for name, data in json.items():
            Log.i("Registering profile '{0}'".format(name))
            profiles[name] = VulkanProfile(registry, name, data, caps)


class VulkanProfilesLibraryGenerator():
    def __init__(self, registry, profiles, debugMessages = False):
        self.registry = registry
        self.profiles = profiles
        self.debugMessages = debugMessages


    def patch_code(self, code):
        # Removes lines with debug messages if they aren't needed
        if self.debugMessages:
            return code
        else:
            lines = code.split('\n')
            patched_lines = []
            for line in lines:
                if not 'VP_DEBUG' in line:
                    patched_lines.append(line)
            return '\n'.join(patched_lines)


    def generate(self, outIncDir, outSrcDir):
        self.generate_h(outIncDir)
        self.generate_cpp(outSrcDir)
        self.generate_hpp(outIncDir)


    def generate_h(self, outDir):
        fileAbsPath = os.path.join(os.path.abspath(outDir), 'vulkan_profiles.h')
        Log.i("Generating '{0}'...".format(fileAbsPath))
        with open(fileAbsPath, 'w') as f:
            f.write(COPYRIGHT_HEADER)
            f.write(H_HEADER)
            f.write(self.gen_profileDefs())
            f.write(API_DEFS)
            f.write(H_FOOTER)


    def generate_cpp(self, outDir):
        fileAbsPath = os.path.join(os.path.abspath(outDir), 'vulkan_profiles.cpp')
        Log.i("Generating '{0}'...".format(fileAbsPath))
        with open(fileAbsPath, 'w') as f:
            f.write(COPYRIGHT_HEADER)
            f.write(CPP_HEADER)
            if self.debugMessages:
                f.write('#include <vulkan/debug/vulkan_profiles.h>\n')
                f.write(DEBUG_MSG_CB_DEFINE)
                f.write(DEBUG_MSG_UTIL_IMPL)
            else:
                f.write('#include <vulkan/vulkan_profiles.h>\n')
            f.write(self.gen_privateImpl())
            f.write(self.gen_publicImpl())


    def generate_hpp(self, outDir):
        fileAbsPath = os.path.join(os.path.abspath(outDir), 'vulkan_profiles.hpp')
        Log.i("Generating '{0}'...".format(fileAbsPath))
        with open(fileAbsPath, 'w') as f:
            f.write(COPYRIGHT_HEADER)
            f.write(HPP_HEADER)
            f.write(self.gen_profileDefs())
            f.write(API_DEFS)
            if self.debugMessages:
                f.write(DEBUG_MSG_CB_DEFINE)
                f.write(DEBUG_MSG_UTIL_IMPL)
            f.write(self.gen_privateImpl())
            f.write(self.gen_publicImpl())
            f.write(HPP_FOOTER)


    def gen_profileDefs(self):
        gen = ''
        for name, profile in sorted(self.profiles.items()):
            uname = name.upper()
            gen += '\n'

            # Add prerequisites
            allRequirements = sorted(profile.versionRequirements) + sorted(profile.extensionRequirements)
            if allRequirements:
                for i, requirement in enumerate(allRequirements):
                    if i == 0:
                        gen += '#if '
                    else:
                        gen += '    '

                    gen += 'defined({0})'.format(requirement)

                    if i < len(allRequirements) - 1:
                        gen += ' && \\\n'
                    else:
                        gen += '\n'

            gen += '#define {0} 1\n'.format(name)
            gen += '#define {0}_NAME "{1}"\n'.format(uname, name)
            gen += '#define {0}_SPEC_VERSION {1}\n'.format(uname, profile.version)
            gen += '#define {0}_MIN_API_VERSION VK_MAKE_VERSION({1})\n'.format(uname, profile.apiVersion.replace(".", ", "))

            if allRequirements:
                gen += '#endif\n'

        return gen


    def gen_privateImpl(self):
        gen = '\n'
        gen += 'namespace detail {\n\n'
        gen += PRIVATE_DEFS
        gen += self.gen_profilePrivateImpl()
        gen += self.gen_profileDescTable()
        gen += PRIVATE_IMPL_BODY
        gen += '\n} // namespace detail\n'
        return self.patch_code(gen)


    def gen_profilePrivateImpl(self):
        gen = ''
        for _, profile in sorted(self.profiles.items()):
            gen += profile.generatePrivateImpl(self.debugMessages)
        return gen


    def gen_dataArrayInfo(self, condition, name):
        if condition:
            return '        &{0}[0], static_cast<uint32_t>(sizeof({0}) / sizeof({0}[0])),\n'.format(name)
        else:
            return '        nullptr, 0,\n'


    def gen_profileDescTable(self):
        gen = '\n'
        gen += 'static const VpProfileDesc vpProfiles[] = {\n'

        for name, profile in sorted(self.profiles.items()):
            uname = name.upper()
            gen += ('#ifdef {0}\n'
                    '    VpProfileDesc{{\n'
                    '        VpProfileProperties{{ {1}_NAME, {1}_SPEC_VERSION }},\n'
                    '        {1}_MIN_API_VERSION,\n').format(name, uname)

            gen += self.gen_dataArrayInfo(profile.capabilities.instanceExtensions, '{0}::instanceExtensions'.format(uname))
            gen += self.gen_dataArrayInfo(profile.capabilities.deviceExtensions, '{0}::deviceExtensions'.format(uname))
            gen += self.gen_dataArrayInfo(profile.fallback, '{0}::fallbacks'.format(uname))
            gen += self.gen_dataArrayInfo(profile.structs.feature, '{0}::featureStructTypes'.format(uname))
            gen += '        {0}::featureDesc,\n'.format(uname)
            gen += self.gen_dataArrayInfo(profile.structs.property, '{0}::propertyStructTypes'.format(uname))
            gen += '        {0}::propertyDesc,\n'.format(uname)
            gen += self.gen_dataArrayInfo(profile.structs.queueFamily, '{0}::queueFamilyStructTypes'.format(uname))
            gen += self.gen_dataArrayInfo(profile.structs.queueFamily, '{0}::queueFamilyDesc'.format(uname))
            gen += self.gen_dataArrayInfo(profile.structs.format, '{0}::formatStructTypes'.format(uname))
            gen += self.gen_dataArrayInfo(profile.structs.format, '{0}::formatDesc'.format(uname))
            gen += '        {0}::chainerDesc,\n'.format(uname)

            gen += ('    },\n'
                    '#endif\n')

        gen += ('};\n'
                'static const uint32_t vpProfileCount = static_cast<uint32_t>(sizeof(vpProfiles) / sizeof(vpProfiles[0]));\n')
        return gen


    def gen_publicImpl(self):
        gen = PUBLIC_IMPL_BODY
        return self.patch_code(gen)


class VulkanProfilesSchemaGenerator():
    def __init__(self, registry):
        self.registry = registry
        self.schema = self.gen_schema()


    def validate(self):
        Log.i("Validating JSON profiles schema...")
        jsonschema.Draft7Validator.check_schema(self.schema)


    def generate(self, outSchema):
        Log.i("Generating '{0}'...".format(outSchema))
        with open(outSchema, 'w') as f:
            f.write(json.dumps(self.schema, indent=4))


    def gen_schema(self):
        definitions = self.gen_baseDefinitions()
        extensions = self.gen_extensions()
        features = self.gen_features(definitions)
        properties = self.gen_properties(definitions)
        formats = self.gen_formats(definitions)
        queueFamilies = self.gen_queueFamilies(definitions)
        versionStr = str(self.registry.headerVersionNumber)

        return OrderedDict({
            "$schema": "http://json-schema.org/draft-07/schema#",
            "$id": "https://schema.khronos.org/vulkan/profiles-0.8.1-{0}.json#".format(str(self.registry.headerVersionNumber.patch)),
            "title": "Vulkan Profiles Schema for Vulkan {0}".format(versionStr),
            "additionalProperties": True,
            "required": [
                "capabilities",
                "profiles"
            ],
            "definitions": definitions,
            "properties": OrderedDict({
                "capabilities": OrderedDict({
                    "description": "The block that specifies the list of capabilities sets.",
                    "type": "object",
                    "additionalProperties": OrderedDict({
                        "type": "object",
                        "additionalProperties": False,
                        "properties": OrderedDict({
                            "extensions": OrderedDict({
                                "description": "The block that stores required extensions.",
                                "type": "object",
                                "additionalProperties": False,
                                "properties": extensions
                            }),
                            "features": OrderedDict({
                                "description": "The block that stores features requirements.",
                                "type": "object",
                                "additionalProperties": False,
                                "properties": features
                            }),
                            "properties": OrderedDict({
                                "description": "The block that stores properties requirements.",
                                "type": "object",
                                "additionalProperties": False,
                                "properties": properties
                            }),
                            "formats": OrderedDict({
                                "description": "The block that store formats capabilities definitions.",
                                "type": "object",
                                "additionalProperties": False,
                                "properties": formats
                            }),
                            "queueFamiliesProperties": OrderedDict({
                                "type": "array",
                                "uniqueItems": True,
                                "items": OrderedDict({
                                    "type": "object",
                                    "additionalProperties": False,
                                    "properties": queueFamilies
                                })
                            })
                        })
                    })
                }),
                "profiles": OrderedDict({
                    "description": "The list of profile definitions.",
                    "type": "object",
                    "additionalProperties": False,
                    "patternProperties": OrderedDict({
                        "^VP_[A-Z0-9]+_[A-Za-z0-9_]+": OrderedDict({
                            "type": "object",
                            "additionalProperties": False,
                            "required": [
                                "label",
                                "description",
                                "version",
                                "api-version",
                                "capabilities"
                            ],
                            "properties": OrderedDict({
                                "version": OrderedDict({
                                    "description": "The revision of the profile.",
                                    "type": "integer"
                                }),
                                "label": OrderedDict({
                                    "description": "The label used to present the profile to the Vulkan developer.",
                                    "type": "string"
                                }),
                                "description": OrderedDict({
                                    "description": "The description of the profile.",
                                    "type": "string"
                                }),
                                "status": OrderedDict({
                                    "description": "The developmet status of the profile: ALPHA, BETA, STABLE or DEPRECATED.",
                                    "$ref": "#/definitions/status"
                                }),
                                "api-version": OrderedDict({
                                    "description": "The Vulkan API version against which the profile is written.",
                                    "type": "string",
                                    "pattern": "^[0-9]+.[0-9]+.[0-9]+$"
                                }),
                                "contributors": OrderedDict({
                                    "type": "object",
                                    "description": "The list of contributors of the profile.",
                                    "additionalProperties": OrderedDict({
                                        "$ref": "#/definitions/contributor"
                                    })
                                }),
                                "history": OrderedDict({
                                    "description": "The version history of the profile file",
                                    "type": "array",
                                    "uniqueItems": True,
                                    "minItems": 1,
                                    "items": OrderedDict({
                                        "type": "object",
                                        "required": [
                                            "revision",
                                            "date",
                                            "author",
                                            "comment"
                                        ],
                                        "properties": OrderedDict({
                                            "revision": OrderedDict({
                                                "type": "integer"
                                            }),
                                            "date": OrderedDict({
                                                "type": "string",
                                                "pattern": "((?:19|20)\\d\\d)-(0?[1-9]|1[012])-([12][0-9]|3[01]|0?[1-9])"
                                            }),
                                            "author": OrderedDict({
                                                "type": "string"
                                            }),
                                            "comment": OrderedDict({
                                                "type": "string"
                                            })
                                        })
                                    })
                                }),
                                "capabilities": OrderedDict({
                                    "description": "The list of required capability sets that can be referenced by a profile.",
                                    "type": "array",
                                    "uniqueItems": True,
                                    "items": OrderedDict({
                                        "anyOf": [
                                            {
                                                "type": "string"
                                            },
                                            {
                                                "type": "array",
                                                "uniqueItems": True,
                                                "items": OrderedDict({
                                                    "type": "string"
                                                })
                                            }
                                        ]
                                    })
                                }),
                                "optionals": OrderedDict({
                                    "description": "The list of optional capability sets that can be referenced by a profile.",
                                    "type": "array",
                                    "uniqueItems": True,
                                    "items": OrderedDict({
                                        "anyOf": [
                                            {
                                                "type": "string"
                                            },
                                            {
                                                "type": "array",
                                                "uniqueItems": True,
                                                "items": OrderedDict({
                                                    "type": "string"
                                                })
                                            }
                                        ]
                                    })
                                }),
                                "fallback": OrderedDict({
                                    "description": "The list of profiles recommended if the checked profile is not supported by the platform.",
                                    "type": "array",
                                    "additionalProperties": False,
                                    "uniqueItems": True,
                                    "items": OrderedDict({
                                        "type": "string"
                                    })
                                }),
                                "contributors": OrderedDict({
                                    "type": "object",
                                    "description": "The list of contributors of the profile.",
                                    "additionalProperties": OrderedDict({
                                        "$ref": "#/definitions/contributor"
                                    })
                                }),
                                "history": OrderedDict({
                                    "description": "The version history of the profile file",
                                    "type": "array",
                                    "uniqueItems": True,
                                    "minItems": 1,
                                    "items": OrderedDict({
                                        "type": "object",
                                        "required": [
                                            "revision",
                                            "date",
                                            "author",
                                            "comment"
                                        ],
                                        "properties": OrderedDict({
                                            "revision": OrderedDict({
                                                "type": "integer"
                                            }),
                                            "date": OrderedDict({
                                                "type": "string",
                                                "pattern": "((?:19|20)\\d\\d)-(0?[1-9]|1[012])-([12][0-9]|3[01]|0?[1-9])"
                                            }),
                                            "author": OrderedDict({
                                                "type": "string"
                                            }),
                                            "comment": OrderedDict({
                                                "type": "string"
                                            })
                                        })
                                    })
                                }),
                            })
                        })
                    })
                })
            })
        })


    def gen_baseDefinitions(self):
        gen = OrderedDict({
            "status": OrderedDict({
                "description": "The development status of the setting. When missing, this property is inherited from parent nodes. If no parent node defines it, the default value is 'STABLE'.",
                "type": "string",
                "enum": [ "ALPHA", "BETA", "STABLE", "DEPRECATED" ]
            }),
            "contributor": OrderedDict({
                "type": "object",
                "additionalProperties": False,
                "required": [
                    "company"
                ],
                "properties": OrderedDict({
                    "company": OrderedDict({
                        "type": "string"
                    }),
                    "email": OrderedDict({
                        "type": "string",
                        "pattern": "^[A-Za-z0-9_.]+@[a-zA-Z0-9-].[a-zA-Z0-9-.]+$"
                    }),
                    "github": OrderedDict({
                        "type": "string",
                        "pattern": "^[A-Za-z0-9_-]+$"
                    }),
                    "contact": OrderedDict({
                        "type": "boolean"
                    })
                })
            }),
            "uint8_t": OrderedDict({
                "type": "integer",
                "minimum": 0,
                "maximum": 255
            }),
            "int32_t": OrderedDict({
                "type": "integer",
                "minimum": -2147483648,
                "maximum": 2147483647
            }),
            "uint32_t": OrderedDict({
                "type": "integer",
                "minimum": 0,
                "maximum": 4294967295
            }),
            "int64_t": OrderedDict({
                "type": "integer"
            }),
            "uint64_t": OrderedDict({
                "type": "integer",
                "minimum": 0
            }),
            "VkDeviceSize": OrderedDict({
                "type": "integer",
                "minimum": 0
            }),
            "char": {
                "type": "string"
            },
            "float": {
                "type": "number"
            },
            "size_t": OrderedDict({
                "type": "integer",
                "minimum": 0
            })
        })
        return gen


    def gen_extensions(self):
        gen = OrderedDict()
        for extName in sorted(self.registry.extensions.keys()):
            gen[extName] = { "type": "integer" }
        return gen


    def gen_type(self, type, definitions):
        if type == 'VkBool32':
            # Simple boolean
            gen = { "type": "boolean" }
        else:
            # All other types are referenced
            gen = { "$ref": "#/definitions/" + type }

        if gen.get("$ref") != None:
            # Generate referenced type, if needed
            if type in definitions:
                # Nothing to do, already defined
                pass
            elif type in self.registry.structs:
                # Generate structure definition
                self.gen_struct(type, definitions)
            elif type in self.registry.enums:
                # Generate enum definition
                self.gen_enum(type, definitions)
            elif type in self.registry.bitmasks:
                # Generate bitmask definition
                self.gen_bitmask(type, definitions)
            else:
                Log.f("Unknown type '{0}'".format(type))

        return gen


    def gen_array(self, type, size, definitions):
        arraySize = self.registry.evalArraySize(size)
        if isinstance(arraySize, list) and len(arraySize) == 1:
            # This is the last dimension of a multi-dimensional array
            # Treat it as one-dimensional from here on
            arraySize = arraySize[0]

        if type == 'char':
            # Character arrays should be handled as strings
            # We assume all are null-terminated, even though the vk.xml doesn't specify that
            # everywhere, but that's probably a bug rather than intentional
            return OrderedDict({
                "type": "string",
                "maxLength": arraySize - 1
            })
        elif isinstance(arraySize, list):
            # Multi-dimensional array
            return OrderedDict({
                "type": "array",
                "items": self.gen_array(type, arraySize[1:], definitions),
                "uniqueItems": False,
                # We don't have information from vk.xml to be able to tell what's the minimum
                # number of items that may need to be specified
                # "minItems": arraySize[0],
                "maxItems": arraySize[0]
            })
        else:
            # One-dimensional array
            return OrderedDict({
                "type": "array",
                "items": self.gen_type(type, definitions),
                "uniqueItems": False,
                # We don't have information from vk.xml to be able to tell what's the minimum
                # number of items that may need to be specified
                # "minItems": arraySize,
                "maxItems": arraySize
            })


    def gen_enum(self, name, definitions):
        enumDef = self.registry.enums[name]

        if len(enumDef.values) > 0:
            values = sorted(enumDef.values)
        else:
            # If the enum has no values then we must add a dummy one
            # in order to produce a valid JSON schema
            values = [ 0 ]

        # Generate definition
        definitions[name] = OrderedDict({
            "enum": values
        })


    def gen_bitmask(self, name, definitions):
        bitmaskDef = self.registry.bitmasks[name]

        if bitmaskDef.bitsType != None:
            # Also generate corresponding bits enum
            self.gen_enum(bitmaskDef.bitsType.name, definitions)
            itemType = { "$ref": "#/definitions/" + bitmaskDef.bitsType.name }
        else:
            # If the bitmask has no bits type then we must add a dummy
            # item type with a single dummy value
            itemType = { "enum": [ 0 ] }

        # Generate definition
        definitions[name] = OrderedDict({
            "type": "array",
            "items": itemType,
            "uniqueItems": True
        })


    def gen_struct(self, name, definitions):
        structDef = self.registry.structs[name]

        # Generate member data
        members = OrderedDict()
        for memberName in sorted(structDef.members.keys()):
            memberDef = structDef.members[memberName]

            if memberDef.type in self.registry.externalTypes and not memberDef.type in definitions:
                # Members with types defined externally and aren't manually defined are ignored
                Log.w("Ignoring member '{0}' in struct '{1}' with external type '{2}'".format(memberName, name, memberDef.type))
                continue

            if memberDef.isArray:
                if memberDef.arraySizeMember != None:
                    # This array is a dynamic one (count + pointer to array) which is not allowed
                    # for return structures. Such structures hence are ill-formed and shouldn't
                    # be included in the schema
                    Log.w("Ignoring member '{0}' in struct '{1}' containing ill-formed pointer to array".format(memberName, name))
                else:
                    members[memberDef.name] = self.gen_array(memberDef.type, memberDef.arraySize, definitions)
            else:
                members[memberDef.name] = self.gen_type(memberDef.type, definitions)

        # Generate definition
        definitions[name] = OrderedDict({
            "type": "object",
            "additionalProperties": False,
            "properties": members
        })


    def gen_structChainDefinitions(self, basename, definitions):
        # Collect unique chainable structures (ignoring aliases)
        structNames = [ basename, basename + '2' ]
        for structName in sorted(self.registry.structs.keys()):
            structDef = self.registry.structs[structName]
            if not structDef.isAlias and basename + '2' in structDef.extends:
                structNames.append(structName)

        # Generate structure definitions and references
        gen = OrderedDict()
        for structName in structNames:
            # Add structure definition and reference
            self.gen_struct(structName, definitions)
            gen[structName] = { "$ref": "#/definitions/" + structName }

            # Add structure references for all alises
            for alias in self.registry.structs[structName].aliases:
                if alias != structName:
                    gen[alias] = gen[structName]

        return gen


    def gen_features(self, definitions):
        return self.gen_structChainDefinitions("VkPhysicalDeviceFeatures", definitions)


    def gen_properties(self, definitions):
        return self.gen_structChainDefinitions("VkPhysicalDeviceProperties", definitions)


    def gen_formats(self, definitions):
        # Add definition for format properties
        definitions['formatProperties'] = OrderedDict({
            "type": "object",
            "additionalProperties": False,
            "properties": self.gen_structChainDefinitions("VkFormatProperties", definitions)
        })

        # Generate references to the format properties definition for each format
        gen = OrderedDict()
        for format in sorted(self.registry.enums['VkFormat'].values):
            gen[format] = OrderedDict({
                "$ref": "#/definitions/formatProperties"
            })
        return gen


    def gen_queueFamilies(self, definitions):
        return self.gen_structChainDefinitions("VkQueueFamilyProperties", definitions)


DOC_MD_HEADER = '''
<!-- markdownlint-disable MD041 -->
<p align="left"><img src="https://vulkan.lunarg.com/img/NewLunarGLogoBlack.png" alt="LunarG" width=263 height=113 /></p>
<p align="left">Copyright (c) 2021-2023 LunarG, Inc.</p>

<p align="center"><img src="./images/logo.png" width=400 /></p>

[![Creative Commons][3]][4]

[3]: https://i.creativecommons.org/l/by-nd/4.0/88x31.png "Creative Commons License"
[4]: https://creativecommons.org/licenses/by-nd/4.0/
'''


class VulkanProfilesDocGenerator():
    def __init__(self, registry, profiles):
        self.registry = registry
        self.profiles = sorted(profiles.values(), key = self.sort_KHR_EXT_first)

        # Determine maximum core version required across all profiles
        self.maxRequiredCoreVersion = max(profile.apiVersionNumber for profile in self.profiles)

        # Collect extensions required by core versions up to the maximum core version required
        # across all profiles so that we can include related data in the relevant tables
        self.coreInstanceExtensions = []
        self.coreDeviceExtensions = []
        for extension in self.registry.extensions.values():
            version = self.registry.getExtensionPromotedToVersion(extension.name)
            if version != None and version.number <= self.maxRequiredCoreVersion:
                if extension.type == 'instance':
                    self.coreInstanceExtensions.append(extension.name)
                elif extension.type == 'device':
                    self.coreDeviceExtensions.append(extension.name)


    def sort_KHR_EXT_first(self, profileOrExtName):
        # Make sure KHR profiles and extensions come first and EXT extensions come next
        key = profileOrExtName.name if isinstance(profileOrExtName, VulkanProfile) else profileOrExtName
        if key[2:7] == '_KHR_':
            return 'A' + key
        elif key[2:7] == '_KHX_':
            return 'B' + key
        elif key[2:7] == '_EXT_':
            return 'C' + key
        else:
            return key


    def generate(self, outDoc):
        Log.i("Generating '{0}'...".format(outDoc))
        with open(outDoc, 'w') as f:
            f.write(self.gen_doc())


    def gen_doc(self):
        gen = DOC_MD_HEADER
        gen += '\n# Vulkan Profiles Definitions\n'
        gen += self.gen_profilesList()
        gen += self.gen_extensions()
        gen += self.gen_features()
        gen += self.gen_limits()
        gen += self.gen_queueFamilies()
        gen += self.gen_formats()
        return gen


    def gen_manPageLink(self, entry, text):
        # The version is irrelevant currently in the man page base link as it gets redirected to
        # the latest version's corresponding page, so we simply use version 1.1 as convention
        return '[{0}](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/{1}.html)'.format(text, entry)


    def gen_table(self, rowHandlers):
        gen = '| Profiles |'
        cellFmt = ' {0} |'
        for profile in self.profiles:
            gen += cellFmt.format(profile.name)
        gen += '\n{0}'.format(re.sub(r"[^|]", '-', gen))
        for row, rowHandler in rowHandlers.items():
            gen += '\n| {0} |'.format(row)
            for profile in self.profiles:
                gen += cellFmt.format(rowHandler(row, profile))
        return gen


    def gen_sectionedTable(self, rowHandlers):
        gen = '| Profiles |'
        cellFmt = ' {0} |'
        for profile in self.profiles:
            gen += cellFmt.format(profile.name)
        gen += '\n{0}'.format(re.sub(r"[^|]", '-', gen))
        for section, sectionRowHandlers in rowHandlers.items():
            gen += '\n| **{0}** |'.format(section)
            for row, rowHandler in sectionRowHandlers.items():
                gen += '\n| {0} |'.format(rowHandler(section, row))
                for profile in self.profiles:
                    gen += cellFmt.format(rowHandler(section, row, profile))
        return gen


    def gen_profilesList(self):
        return '\n## Vulkan Profiles List\n\n{0}\n'.format(self.gen_table(OrderedDict({
            'Label': lambda _, profile : profile.label,
            'Description': lambda _, profile : profile.description,
            'Version': lambda _, profile : profile.version,
            'Required API version': lambda _, profile : profile.apiVersion,
            'Fallback profiles': lambda _, profile : ', '.join(profile.fallback) if profile.fallback != None else '-'
        })))


    def gen_extension(self, section, extension, profile = None):
        # If no profile was specified then this is the first column so return the extension name
        # with a link to the extension's manual page
        if profile is None:
            return self.gen_manPageLink(extension, extension)

        # If it's an extension explicitly required by the profile then this is a supported extension
        if extension in profile.capabilities.extensions:
            return ':heavy_check_mark:'

        # Otherwise check if this extension has been promoted to a core API version that the profile requires
        version = self.registry.getExtensionPromotedToVersion(extension)
        # If core API version found and is required by the profile then this extension is supported as being core
        if version != None and version.number <= profile.apiVersionNumber:
            return str(version.number) + ' Core'

        # Otherwise it's unsupported
        return ':x:'


    def gen_extensions(self):
        # Collect instance extensions defined by the profiles
        instanceExtensions = self.coreInstanceExtensions + list(itertools.chain(*[
            profile.capabilities.instanceExtensions.keys() for profile in self.profiles
        ]))
        instanceExtensions.sort(key = self.sort_KHR_EXT_first)

        # Collect device extensions defined by the profiles
        deviceExtensions = self.coreDeviceExtensions + list(itertools.chain(*[
            profile.capabilities.deviceExtensions.keys() for profile in self.profiles
        ]))
        deviceExtensions.sort(key = self.sort_KHR_EXT_first)

        # Generate table legend
        legend = (
            '* :heavy_check_mark: indicates that the extension is defined in the profile\n'
            '* "X.X Core" indicates that the extension is not defined in the profile but '
            'the extension is promoted to the specified core API version that is smaller than '
            'or equal to the minimum required API version of the profile\n'
            '* :x: indicates that the extension is neither defined in the profile nor it is '
            'promoted to a core API version that is smaller than or equal to the minimum '
            'required API version of the profile\n'
        )

        # Generate table
        table = self.gen_sectionedTable(OrderedDict({
            'Instance extensions': OrderedDict({ row: self.gen_extension for row in instanceExtensions }),
            'Device extensions': OrderedDict({ row: self.gen_extension for row in deviceExtensions })
        }))
        return '\n## Vulkan Profiles Extensions\n\n{0}\n{1}\n'.format(legend, table)


    def has_nestedFeatureData(self, data):
        for key in data:
            if not isinstance(data[key], bool):
                return True
        return None


    def formatFeatureSupport(self, supported, struct, section):
        structDef = self.registry.structs[struct]
        # VkPhysicalDeviceVulkan11Features is defined in Vulkan 1.2, but actually it defines Vulkan 1.1 features
        if struct == 'VkPhysicalDeviceVulkan11Features':
            where = 'Vulkan 1.1'
            isExactMatch = (section == where)
        elif structDef.definedByVersion != None:
            where = 'Vulkan {0}'.format(str(structDef.definedByVersion))
            isExactMatch = (section == where)
        elif len(structDef.definedByExtensions) > 0:
            where = '/'.join(structDef.definedByExtensions)
            isExactMatch = (section in structDef.definedByExtensions)
        else:
            where = 'Vulkan 1.0'
            isExactMatch = (section == where)
        if supported:
            if isExactMatch:
                return '<span title="defined in {0} ({1})">:heavy_check_mark:</span>'.format(struct, where)
            else:
                return '<span title="equivalent defined in {0} ({1})">:warning:</span>'.format(struct, where)
        else:
            return ':x:'


    def getFeatureStructSynonyms(self, struct, member):
        structDef = self.registry.structs[struct]
        if structDef.definedByVersion != None:
            # For 1.1+ core features we always have two structures defining the feature, one is
            # the feature specific structure, the other is VkPhysicalDeviceVulkanXXFeatures
            if struct == 'VkPhysicalDeviceVulkan11Features':
                # VkPhysicalDeviceVulkan11Features is defined in Vulkan 1.2, but actually it
                # defines Vulkan 1.1 features
                version = self.registry.versions['VK_VERSION_1_1']
            else:
                # For other structures find the version defining the structure
                for version in self.registry.versions.values():
                    if version.number == structDef.definedByVersion:
                        break
            # Return all the structures defining this feature member
            return version.features[member].structs
        else:
            # In all other cases we're talking about a non-promoted extension, as the structure
            # we receive here is always a non-alias structure, so we can simply return the
            # aliases of the structure
            return structDef.aliases


    def getFeatureStructForManPageLink(self, struct, member):
        # We don't want to link to the man page VkPhysicalDeviceVulkanXXFeatures structures,
        # instead we prefer to use the more specific non-alias structure if possible
        for alias in self.getFeatureStructSynonyms(struct, member):
            if re.match(r"^VkPhysicalDeviceVulkan[0-9]+Features$", alias) is None:
                structDef = self.registry.structs[alias]
                if not structDef.isAlias:
                    struct = alias
        return struct


    def gen_feature(self, struct, section, member, profile = None):
        # If no profile was specified then this is the first column so return the member name
        # with a link to the encompassing structure's manual page
        if profile is None:
            return self.gen_manPageLink(self.getFeatureStructForManPageLink(struct, member),
                                        member)

        # If this feature struct member is defined in the profile as is, consider it supported
        if struct in profile.capabilities.features:
            featureStruct = profile.capabilities.features[struct]
            if member in featureStruct:
                return self.formatFeatureSupport(featureStruct[member], struct, section)

        # If the struct is VkPhysicalDeviceFeatures then check if the feature is defined in
        # VkPhysicalDeviceFeatures2 or VkPhysicalDeviceFeatures2KHR for the profile and then
        # consider it supported
        if struct == 'VkPhysicalDeviceFeatures':
            for wrapperStruct in [ 'VkPhysicalDeviceFeatures2', 'VkPhysicalDeviceFeatures2KHR' ]:
                if wrapperStruct in profile.capabilities.features:
                    featureStruct = profile.capabilities.features[wrapperStruct]['features']
                    if member in featureStruct:
                        return self.formatFeatureSupport(featureStruct[member], struct, section)

        # If the struct has aliases and the feature struct member is defined in the profile in
        # one of those, consider it supported
        for alias in self.getFeatureStructSynonyms(struct, member):
            if alias in profile.capabilities.features:
                featureStruct = profile.capabilities.features[alias]
                if member in featureStruct:
                    return self.formatFeatureSupport(featureStruct[member], alias, section)

        return self.formatFeatureSupport(False, struct, section)


    def gen_featuresSection(self, features, definedFeatures, sectionHeader, tableData):
        # Go through defined feature structures
        for definedFeatureStructName, definedFeatureList in definedFeatures.items():
            # Go through defined features within those structures
            for definedFeature in definedFeatureList:
                # Check if there's a feature with a matching name in the features to consider
                if definedFeature in features.keys():
                    feature = features[definedFeature]
                    # Check that the feature structure actually matches one of the structures
                    # this feature is defined in (this is needed because the registry xml doesn't
                    # prevent multiple structures defining features with identical names so we
                    # have to check whether we actually talk about a synonym or a completely
                    # different feature with the same name)
                    if definedFeatureStructName in feature.structs:
                        if not sectionHeader in tableData:
                            tableData[sectionHeader] = OrderedDict()
                        # Feature is defined, add it to the table
                        tableData[sectionHeader][definedFeature] = functools.partial(self.gen_feature, definedFeatureStructName)


    def gen_features(self):
        # Merge all feature references across the profiles to collect the relevant features to look at
        definedFeatures = dict()
        for profile in self.profiles:
            for featureStructName, features in profile.capabilities.features.items():
                # VkPhysicalDeviceFeatures2 is an exception, as it contains a nested structure
                # No other structure is allowed to have this
                if featureStructName in [ 'VkPhysicalDeviceFeatures2', 'VkPhysicalDeviceFeatures2KHR' ]:
                    featureStructName = 'VkPhysicalDeviceFeatures'
                    features = features['features']
                elif self.has_nestedFeatureData(features):
                    Log.f("Unexpected nested feature data in profile '{0}' structure '{1}'".format(profile.name, featureStructName))
                # If this is an alias structure then find the non-alias one and use that
                featureStructName = self.registry.getNonAliasTypeName(featureStructName, self.registry.structs)
                # Copy defined feature structure data
                if not featureStructName in definedFeatures:
                    definedFeatures[featureStructName] = []
                definedFeatures[featureStructName].extend(features.keys())

        tableData = OrderedDict()

        # First, go through core features
        for version in sorted(self.registry.versions.values(), key = lambda version: version.number):
            self.gen_featuresSection(version.features, definedFeatures, 'Vulkan ' + str(version.number), tableData)

        # Then, go through extensions
        for extension in sorted(self.registry.extensions.values(), key = lambda extension: self.sort_KHR_EXT_first(extension.name)):
            self.gen_featuresSection(extension.features, definedFeatures, extension.name, tableData)

        # Sort individual features within the sections by name
        for sectionName in tableData.keys():
            tableData[sectionName] = OrderedDict(sorted(tableData[sectionName].items()))

        # TODO: Currently we don't include features that are required by the minimum required API
        # version of a profile, or features required by extensions required by the profile, as
        # that would necessitate the inclusion of the information currently only available
        # textually in the "Feature Requirements" section of the Vulkan Specification
        disclaimer = (
            '> **NOTE**: The table below only contains features explicitly defined by the '
            'corresponding profile. Further features may be supported by the profiles in '
            'accordance to the requirements defined in the "Feature Requirements" section '
            'of the appropriate version of the Vulkan API Specification.'
        )

        # Generate table legend
        legend = (
            '* :heavy_check_mark: indicates that the feature is defined in the profile (hover '
            'over the symbol to view the structure and corresponding extension or core API '
            'version where the feature is defined in the profile)\n'
            '* :warning: indicates that the feature is not defined in the profile but an '
            'equivalent feature is (hover over the symbol to view the structure and '
            'corresponding extension or core API version where the feature is defined in the '
            'profile)\n'
            '* :x: indicates that neither the feature nor an equivalent feature is defined in '
            'the profile\n'
        )

        # Generate table
        table = self.gen_sectionedTable(tableData)
        return '\n## Vulkan Profile Features\n\n{0}\n\n{1}\n{2}\n'.format(disclaimer, legend, table)


    def formatValue(self, value):
        if type(value) == bool:
            # Boolean
            return 'VK_TRUE' if value else 'VK_FALSE'
        elif type(value) == dict:
            # Structure, match the Vulkan Specification's formatting
            return '({0})'.format(','.join(str(el) for el in value.values()))
        elif type(value) == list:
            if len(value) == 0:
                # Empty array, not much to return
                return '-'
            elif type(value[0]) == str:
                # Bitmask, match the Vulkan Specification's formatting
                return '({0})'.format(' \| '.join(value))
            else:
                # Array, match the Vulkan Specification's formatting
                return '({0})'.format(','.join(str(el) for el in value))
        else:
            return str(value)


    def formatProperty(self, value, struct, section = None):
        structDef = self.registry.structs[struct]
        # VkPhysicalDeviceVulkan11Properties is defined in Vulkan 1.2, but actually it defines Vulkan 1.1 features
        if struct == 'VkPhysicalDeviceVulkan11Properties':
            where = 'Vulkan 1.1'
            isExactMatch = (section == where)
        elif structDef.definedByVersion != None:
            where = 'Vulkan {0}'.format(str(structDef.definedByVersion))
            isExactMatch = (section == where)
        elif len(structDef.definedByExtensions) > 0:
            where = '/'.join(structDef.definedByExtensions)
            isExactMatch = (section in structDef.definedByExtensions)
        else:
            where = 'Vulkan 1.0'
            isExactMatch = (section == where)
        if isExactMatch or section == None:
            return '<span title="defined in {0} ({1})">{2}</span>'.format(struct, where, self.formatValue(value))
        else:
            return '<span title="equivalent defined in {0} ({1})">_{2}_</span>'.format(struct, where, self.formatValue(value))


    def formatLimitName(self, struct, member):
        structDef = self.registry.structs[struct]
        memberDef = structDef.members[member]
        limittype = memberDef.limittype

        if limittype in [ None, 'noauto', 'bitmask' ]:
            return member
        elif limittype == 'exact':
            return member + ' (exact)'
        elif limittype == 'max':
            return member + ' (max)'
        elif limittype == 'max,pot' or limittype == 'pot,max':
            return member + ' (max,pot)'
        elif limittype in [ 'min' ]:
            return member + ' (min)'
        elif limittype == 'min,pot' or limittype == 'pot,min':
            return member + ' (min,pot)'
        elif limittype == 'min,mul' or limittype == 'mul,min':
            return member + ' (min,mul)'
        elif limittype == 'bits':
            return member + ' (bits)'
        elif limittype == 'range':
            return member + ' (min-max)'
        else:
            Log.f("Unexpected limittype '{0}'".format(limittype))


    def getLimitStructSynonyms(self, struct, member):
        structDef = self.registry.structs[struct]
        if structDef.definedByVersion != None:
            # For 1.1+ core limits we always have two structures defining the limit, one is
            # the limit specific structure, the other is VkPhysicalDeviceVulkanXXProperties
            if struct == 'VkPhysicalDeviceVulkan11Properties':
                # VkPhysicalDeviceVulkan11Properties is defined in Vulkan 1.2, but actually it
                # defines Vulkan 1.1 limits
                version = self.registry.versions['VK_VERSION_1_1']
            else:
                # For other structures find the version defining the structure
                for version in self.registry.versions.values():
                    if version.number == structDef.definedByVersion:
                        break
            # Return all the structures defining this limit member
            return version.limits[member].structs
        else:
            # In all other cases we're talking about a non-promoted extension, as the structure
            # we receive here is always a non-alias structure, so we can simply return the
            # aliases of the structure
            return structDef.aliases


    def getLimitStructForManPageLink(self, struct, member):
        # If the structure at hand is VkPhysicalDeviceProperties then we should rather link
        # to the underlying nested structure that actually defines the limit
        if struct == 'VkPhysicalDeviceProperties':
            structs = self.registry.versions['VK_VERSION_1_0'].limits[member].structs
            for nestedStruct in [ 'VkPhysicalDeviceLimits', 'VkPhysicalDeviceSparseProperties' ]:
                if nestedStruct in structs:
                    return nestedStruct

        # We don't want to link to the man page VkPhysicalDeviceVulkanXXProperties structures,
        # instead we prefer to use the more specific non-alias structure if possible
        for alias in self.getLimitStructSynonyms(struct, member):
            if re.match(r"^VkPhysicalDeviceVulkan[0-9]+Properties$", alias) is None:
                structDef = self.registry.structs[alias]
                if not structDef.isAlias:
                    struct = alias
        return struct


    def gen_limit(self, struct, section, member, profile = None):
        # If no profile was specified then this is the first column so return the member name
        # decorated with the corresponding limittype specific info and a link to the
        # encompassing structure's manual page
        if profile is None:
            return self.gen_manPageLink(self.getLimitStructForManPageLink(struct, member),
                                        self.formatLimitName(struct, member))

        # If this limit/property struct member is defined in the profile as is, include it
        if struct in profile.capabilities.properties:
            limitStruct = profile.capabilities.properties[struct]
            if member in limitStruct:
                return self.formatProperty(limitStruct[member], struct, section)

        # If the struct is VkPhysicalDeviceLimits or VkPhysicalDeviceSparseProperties then check
        # if the limit/property is defined somewhere nested in VkPhysicalDeviceProperties,
        # VkPhysicalDeviceProperties2, or VkPhysicalDeviceProperties2KHR for the profile then
        # include it
        if struct == 'VkPhysicalDeviceLimits' or struct == 'VkPhysicalDeviceSparseProperties':
            if struct == 'VkPhysicalDeviceLimits':
                memberStruct = 'limits'
            else:
                memberStruct = 'sparseProperties'
            propertyStruct = None
            if 'VkPhysicalDeviceProperties' in profile.capabilities.properties:
                propertyStructName = 'VkPhysicalDeviceProperties'
                propertyStruct = profile.capabilities.properties[propertyStructName]
            for wrapperStruct in [ 'VkPhysicalDeviceProperties2', 'VkPhysicalDeviceProperties2KHR' ]:
                if wrapperStruct in profile.capabilities.properties:
                    propertyStructName = wrapperStruct
                    propertyStruct = profile.capabilities.properties[wrapperStruct]['properties']
            if propertyStruct != None and memberStruct != 'sparseProperties':
                limitStruct = propertyStruct[memberStruct]
                if member in limitStruct:
                    return self.formatProperty(limitStruct[member], propertyStructName, section)

        # If the struct has aliases and the limit/property struct member is defined in the profile
        # in one of those then include it
        for alias in self.getLimitStructSynonyms(struct, member):
            if alias in profile.capabilities.properties:
                limitStruct = profile.capabilities.properties[alias]
                if member in limitStruct and limitStruct[member]:
                    return self.formatProperty(limitStruct[member], alias, section)

        return '-'


    def gen_limitsSection(self, limits, definedLimits, sectionHeader, tableData):
        # Go through defined limit/property structures
        for definedLimitStructName, definedLimitList in definedLimits.items():
            # Go through defined limits within those structures
            for definedLimit in definedLimitList:
                # Check if there's a limit with a matching name in the limits to consider
                if definedLimit in limits.keys():
                    limit = limits[definedLimit]
                    # Check that the limit/property structure actually matches one of the
                    # structures this limit is defined in (this is needed because the registry xml
                    # doesn't prevent multiple structures defining limits/properties with
                    # identical names so we have to check whether we actually talk about a synonym
                    # or a completely different limit/property with the same name)
                    if definedLimitStructName in limit.structs:
                        if not sectionHeader in tableData:
                            tableData[sectionHeader] = OrderedDict()
                        # Limit/property is defined, add it to the table
                        tableData[sectionHeader][definedLimit] = functools.partial(self.gen_limit, definedLimitStructName)


    def gen_limits(self):
        # Merge all limit/property references across the profiles to collect the relevant limits to look at
        definedLimits = dict()
        for profile in self.profiles:
            for propertyStructName, properties in profile.capabilities.properties.items():
                # VkPhysicalDeviceProperties and VkPhysicalDeviceProperties2 are exceptions,
                # need custom handling due to only using their nested structures
                if propertyStructName in [ 'VkPhysicalDeviceProperties2', 'VkPhysicalDeviceProperties2KHR' ]:
                    propertyStructName = 'VkPhysicalDeviceProperties'
                    properties = properties['properties']
                if propertyStructName == 'VkPhysicalDeviceProperties':
                    for member, struct in { 'limits': 'VkPhysicalDeviceLimits', 'sparseProperties': 'VkPhysicalDeviceSparseProperties' }.items():
                        if member in properties:
                            definedLimits[struct] = properties[member].keys()
                    continue

                # If this is an alias structure then find the non-alias one and use that
                propertyStructName = self.registry.getNonAliasTypeName(propertyStructName, self.registry.structs)
                # Copy defined limit/property structure data
                if not propertyStructName in definedLimits:
                    definedLimits[propertyStructName] = []
                definedLimits[propertyStructName].extend(properties.keys())

        tableData = OrderedDict()

        # First, go through core limits/properties
        for version in sorted(self.registry.versions.values(), key = lambda version: version.number):
            self.gen_limitsSection(version.limits, definedLimits, 'Vulkan ' + str(version.number), tableData)

        # Then, go through extensions
        for extension in sorted(self.registry.extensions.values(), key = lambda extension: self.sort_KHR_EXT_first(extension.name)):
            self.gen_limitsSection(extension.limits, definedLimits, extension.name, tableData)

        # Sort individual limits within the sections by name
        for sectionName in tableData.keys():
            tableData[sectionName] = OrderedDict(sorted(tableData[sectionName].items()))

        # TODO: Currently we don't include limits/properties that are required by the minimum
        # required API version of a profile, or limits/properties required by extensions required
        # by the profile, as that would necessitate the inclusion of information currently only
        # available textually in the "Limit Requirements" section of the Vulkan Specification
        disclaimer = (
            '> **NOTE**: The table below only contains properties/limits explicitly defined '
            'by the corresponding profile. Further properties/limits may be supported by the '
            'profiles in accordance to the requirements defined in the "Limit Requirements" '
            'section of the appropriate version of the Vulkan API Specification.'
        )

        # Generate table legend
        legend = (
            '* "valueWithRegularFont" indicates that the limit/property is defined in the profile '
            '(hover over the value to view the structure and corresponding extension or core API '
            'version where the limit/property is defined in the profile)\n'
            '* "_valueWithItalicFont_" indicates that the limit/property is not defined in the profile '
            'but an equivalent limit/property is (hover over the symbol to view the structure '
            'and corresponding extension or core API version where the limit/property is defined '
            'in the profile)\n'
            '* "-" indicates that neither the limit/property nor an equivalent limit/property is '
            'defined in the profile\n'
        )

        # Generate table
        table = self.gen_sectionedTable(tableData)
        return '\n## Vulkan Profile Limits (Properties)\n\n{0}\n\n{1}\n{2}\n'.format(disclaimer, legend, table)


    def gen_queueFamily(self, index, struct, section, member, profile = None):
        # If no profile was specified then this is the first column so return the member name
        # decorated with the corresponding limittype specific info and a link to the
        # encompassing structure's manual page
        if profile is None:
            return self.gen_manPageLink(struct, self.formatLimitName(struct, member))

        # If this profile doesn't even define this queue family index then early out
        if len(profile.capabilities.queueFamiliesProperties) <= index:
            return ''

        # If this queue family property struct member is defined in the profile as is, include it
        if struct in profile.capabilities.queueFamiliesProperties[index]:
            propertyStruct = profile.capabilities.queueFamiliesProperties[index][struct]
            if member in propertyStruct:
                return self.formatProperty(propertyStruct[member], struct)

        # If the struct is VkPhysicalDeviceQueueFamilyProperties then check if the feature is
        # defined in VkPhysicalDeviceQueueFamilyProperties2 or VkPhysicalDeviceQueueFamilyProperties2KHR
        # for the profile and then include it
        if struct == 'VkPhysicalDeviceQueueFamilyProperties':
            for wrapperStruct in [ 'VkPhysicalDeviceQueueFamilyProperties2', 'VkPhysicalDeviceQueueFamilyProperties2KHR' ]:
                if wrapperStruct in profile.capabilities.queueFamiliesProperties[index]:
                    propertyStruct = profile.capabilities.queueFamiliesProperties[index][wrapperStruct]['queueFamilyProperties']
                    if member in propertyStruct and propertyStruct[member]:
                        return self.formatProperty(propertyStruct[member], wrapperStruct)

        # If the struct has aliases and the property struct member is defined in the profile
        # in one of those then include it
        structDef = self.registry.structs[struct]
        for alias in structDef.aliases:
            if alias in profile.capabilities.queueFamiliesProperties[index]:
                propertyStruct = profile.capabilities.queueFamiliesProperties[index][alias]
                if member in propertyStruct and propertyStruct[member]:
                    return self.formatProperty(propertyStruct[member], alias)

        return '-'


    def gen_queueFamilies(self):
        # Merge all queue family property references across the profiles to collect the relevant
        # properties to look at for each queue family definition index
        definedQueueFamilies = []
        for profile in self.profiles:
            for index, queueFamily in enumerate(profile.capabilities.queueFamiliesProperties):
                definedQueueFamilyProperties = OrderedDict()
                for structName, properties in queueFamily.items():
                    # VkPhysicalDeviceQueueFamilies2 is an exception, as it contains a nested structure
                    # No other structure is allowed to have this
                    if structName in [ 'VkPhysicalDeviceQueueFamilyProperties2', 'VkPhysicalDeviceQueueFamilyProperties2KHR']:
                        structName = 'VkPhysicalDeviceQueueFamilyProperties'
                        properties = properties['queueFamilyProperties']
                    # If this is an alias structure then find the non-alias one and use that
                    structName = self.registry.getNonAliasTypeName(structName, self.registry.structs)
                    # Copy defined limit/property structure data
                    if not structName in definedQueueFamilyProperties:
                        definedQueueFamilyProperties[structName] = []
                    definedQueueFamilyProperties[structName].extend(sorted(properties.keys()))
                # Add queue family to the list
                if len(definedQueueFamilies) <= index:
                    definedQueueFamilies.append(dict())
                definedQueueFamilies[index].update(definedQueueFamilyProperties)

        # Construct table data
        tableData = OrderedDict()
        for index, queueFamilyProperties in enumerate(definedQueueFamilies):
            section = tableData['Queue family #' + str(index)] = OrderedDict()
            for structName, members in queueFamilyProperties.items():
                section.update({ row: functools.partial(self.gen_queueFamily, index, structName) for row in members })

        # Generate table legend
        legend = (
            '* "valueWithRegularFont" indicates that the queue family property is defined in the '
            'profile (hover over the value to view the structure and corresponding extension or '
            'core API version where the queue family property is defined in the profile)\n'
            '* "_valueWithItalicFont_" indicates that the queue family property is not defined in the '
            'profile but an equivalent queue family property is (hover over the symbol to view '
            'the structure and corresponding extension or core API version where the queue family '
            'property is defined in the profile)\n'
            '* "-" indicates that neither the queue family property nor an equivalent queue '
            'family property is defined in the profile\n'
            '* Empty cells next to the properties of a particular queue family definition section '
            'indicate that the profile does not have a corresponding queue family definition\n'
        )

        # Generate table
        table = self.gen_sectionedTable(tableData)
        return '\n## Vulkan Profile Queue Families\n\n{0}\n{1}\n'.format(legend, table)


    def getFormatStructForManPageLink(self, struct):
        # We prefer returning VkFormatProperties3 instead of VkFormatProperties as even though
        # they are technically not strictly aliases, the former is the one that should be used
        # going forward and the feature flags are anyway defined to be usable as synonyms for
        # the legacy 32-bit flags
        return 'VkFormatProperties3' if struct == 'VkFormatProperties' else struct


    def gen_format(self, format, struct, section, member, profile = None):
        # If no profile was specified then this is the first column so return the member name
        # decorated with the corresponding limittype specific info and a link to the
        # encompassing structure's manual page
        if profile is None:
            return self.gen_manPageLink(self.getFormatStructForManPageLink(struct),
                                        self.formatLimitName(struct, member))

        # If this profile doesn't even define this format then early out
        if not format in profile.capabilities.formats:
            # Before doing so, though, we have to check whether any of the aliases of the format
            # are defined by the profile
            formatAliases = self.registry.enums['VkFormat'].aliasValues
            if not format in formatAliases or not formatAliases[format] in profile.capabilities.formats:
                return ''

        # If this format property struct member is defined in the profile as is, include it
        if struct in profile.capabilities.formats[format]:
            propertyStruct = profile.capabilities.formats[format][struct]
            if member in propertyStruct:
                return self.formatProperty(propertyStruct[member], struct)

        # If the struct is VkFormatProperties then 'member' also contains the trimmed name of
        # the flag bit to check for, so we check for that, or any of its aliases
        if struct == 'VkFormatProperties':
            for alternative in [ 'VkFormatProperties', 'VkFormatProperties2', 'VkFormatProperties2KHR', 'VkFormatProperties3', 'VkFormatProperties3KHR' ]:
                if alternative in profile.capabilities.formats[format]:
                    propertyStruct = profile.capabilities.formats[format][alternative]
                    # VkFormatProperties2[KHR] wrap the real structure in a member
                    if 'formatProperties' in propertyStruct:
                        propertyStruct = propertyStruct['formatProperties']
                    if member in propertyStruct:
                        return self.formatProperty(propertyStruct[member], alternative)

        # If the struct has aliases and the property struct member is defined in the profile
        # in one of those then include it
        structDef = self.registry.structs[struct]
        for alias in structDef.aliases:
            if alias in profile.capabilities.formats[format]:
                propertyStruct = profile.capabilities.formats[format][alias]
                if member in propertyStruct and propertyStruct[member]:
                    return self.formatProperty(propertyStruct[member], alias)

        return '-'


    def gen_formats(self):
        # Merge all format property references across the profiles to collect the relevant
        # properties to look at for each format
        definedFormats = dict()
        for profile in self.profiles:
            for format, formatProperties in profile.capabilities.formats.items():
                # This may be an alias of a format name, so get the real name
                formatAliases = self.registry.enums['VkFormat'].aliasValues
                format = formatAliases[format] if format in formatAliases else format

                definedFormatProperties = OrderedDict()
                for structName, properties in formatProperties.items():
                    # VkFormatProperties, VkFormatProperties2, and VkFormatProperties3 are special
                    if structName in [ 'VkFormatProperties2', 'VkFormatProperties2KHR' ]:
                        structName = 'VkFormatProperties'
                        properties = properties['formatProperties']
                    if structName in [ 'VkFormatProperties3', 'VkFormatProperties3KHR' ]:
                        structName = 'VkFormatProperties'
                    # If this is an alias structure then find the non-alias one and use that
                    structName = self.registry.getNonAliasTypeName(structName, self.registry.structs)
                    # Copy defined format property structure data
                    if not structName in definedFormatProperties:
                        definedFormatProperties[structName] = []
                    definedFormatProperties[structName].extend(sorted(properties.keys()))

                # Add format information
                if not format in definedFormats:
                    definedFormats[format] = OrderedDict()
                definedFormats[format].update(definedFormatProperties)


        # Construct table data
        tableData = OrderedDict()
        for format in sorted(definedFormats.keys()):
            section = tableData[format] = OrderedDict()
            for structName, members in definedFormats[format].items():
                section.update({ row: functools.partial(self.gen_format, format, structName) for row in members })

        # TODO: Currently we don't include format properties that are required by the minimum
        # required API version of a profile, or those required by extensions required by the
        # profile, as that would necessitate the inclusion of information currently only
        # available textually in the "Required Format Support" section of the Vulkan Specification
        disclaimer = (
            '> **NOTE**: The table below only contains formats and properties explicitly defined '
            'by the corresponding profile. Further formats and properties may be supported by the '
            'profiles in accordance to the requirements defined in the "Required Format Support" '
            'section of the appropriate version of the Vulkan API Specification.'
        )

        # Generate table legend
        legend = (
            '* "valueWithRegularFont" indicates that the format property is defined in the '
            'profile (hover over the value to view the structure and corresponding extension or '
            'core API version where the format property is defined in the profile)\n'
            '* "_valueWithItalicFont_" indicates that the format property is not defined in the '
            'profile but an equivalent format property is (hover over the symbol to view the '
            'structure and corresponding extension or core API version where the format property '
            'is defined in the profile)\n'
            '* "-" indicates that neither the format property nor an equivalent format property '
            'is defined in the profile\n'
            '* Empty cells next to the properties of a particular format definition section '
            'indicate that the profile does not have a corresponding format definition\n'
        )

        # Generate table
        table = self.gen_sectionedTable(tableData)
        return '\n## Vulkan Profile Formats\n\n{0}\n\n{1}\n{2}\n'.format(disclaimer, legend, table)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    parser.add_argument('--registry', '-r', action='store', required=True,
                        help='Use specified registry file instead of vk.xml')
    parser.add_argument('--input', '-i', action='store', required=True,
                        help='Path to directory with profiles.')
    parser.add_argument('--output-library-inc', action='store',
                        help='Output include directory for profile library')
    parser.add_argument('--output-library-src', action='store',
                        help='Output source directory for profile library')
    parser.add_argument('--output-schema', action='store',
                        help='Output file for JSON profile schema')
    parser.add_argument('--output-doc', action='store',
                        help='Output file for profiles markdown documentation')
    parser.add_argument('--validate', '-v', action='store_true',
                        help='Validate generated JSON profile schema and JSON profiles against the schema')
    parser.add_argument('--debug', '-d', action='store_true',
                        help='Also generate library variant with debug messages')

    args = parser.parse_args()

    if args.output_library_inc is None and args.output_schema is None and args.output_doc is None and not args.validate:
        parser.print_help()
        exit()

    if args.output_library_inc != None or args.output_library_src != None:
        if args.registry is None or args.input is None or args.output_library_inc is None or args.output_library_src is None:
            Log.e("Generating the profile library requires specifying --registry, --input, --output-library-inc and --output-library-src arguments")
            parser.print_help()
            exit()

    if args.output_schema != None:
        if args.registry is None:
            Log.e("Generating the profile schema requires specifying --registry and ---output-schema arguments")
            parser.print_help()
            exit()

    if args.output_doc != None:
        if args.registry is None or args.input is None:
            Log.e("Generating the profile schema requires specifying --registry, --input and --output-doc arguments")
            parser.print_help()
            exit()

    schema = None

    if args.registry != None:
        registry = VulkanRegistry(args.registry)

    if args.output_schema != None or args.validate:
        generator = VulkanProfilesSchemaGenerator(registry)
        if args.output_schema is not None:
            generator.generate(args.output_schema)
        if args.validate:
            generator.validate()
            schema = generator.schema

    if args.input != None:
        profiles = VulkanProfiles.loadFromDir(registry, args.input, args.validate, schema)

    if args.output_library_inc != None:
        generator = VulkanProfilesLibraryGenerator(registry, profiles)
        generator.generate(args.output_library_inc, args.output_library_src)
        if args.debug:
            generator = VulkanProfilesLibraryGenerator(registry, profiles, True)
            generator.generate(args.output_library_inc + '/debug', args.output_library_src + '/debug')

    if args.output_doc != None:
        generator = VulkanProfilesDocGenerator(registry, profiles)
        generator.generate(args.output_doc)
