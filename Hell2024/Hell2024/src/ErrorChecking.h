#pragma once

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
#include <iostream>

inline void VK_CHECK(VkResult err) {
    if (err) {
        std::cout << "Detected Vulkan error: " << err << "\n";
        abort();
    }
}