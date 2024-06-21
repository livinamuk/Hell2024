#pragma once
#include <vulkan/vulkan.h>

enum class VertexDescriptionType { 
    POSITION_NORMAL_TEXCOORD, 
    POSITION_TEXCOORD, 
    POSITION_NORMAL, 
    POSITION, 
    ALL, 
    ALL_WEIGHTED,
    NONE
};

struct VertexInputDescription {
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;
    VkPipelineVertexInputStateCreateFlags flags = 0;
    void Reset() {
        bindings.clear();
        attributes.clear();
        flags = 0;
    }
};