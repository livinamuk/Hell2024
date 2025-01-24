#pragma once
#include <Vulkan/vulkan.h>
#include <vector>
#include <string>
#include "VK_allocation.hpp"
#include "VK_shader.hpp"

struct AccelerationStructure {
    VkAccelerationStructureKHR handle;
    uint64_t deviceAddress = 0;
    VkDeviceMemory memory;
    AllocatedBuffer buffer;
};

struct RayTracingScratchBuffer {
    uint64_t deviceAddress = 0;
    AllocatedBuffer handle;
    VkDeviceMemory memory = VK_NULL_HANDLE;
};

struct Raytracer {
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    AllocatedBuffer raygenShaderBindingTable;
    AllocatedBuffer missShaderBindingTable;
    AllocatedBuffer hitShaderBindingTable;

    struct ShaderStack {
        VkShaderModule rayGen = nullptr;
        VkShaderModule rayMiss = nullptr;
        VkShaderModule closestHit = nullptr;
        VkShaderModule rayshadowMiss = nullptr;
    } shaderStack;

    VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
    VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
    VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
    VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};

   /* void LoadShaders(VkDevice device, std::string rayGenPath, std::string missPath, std::string shadowMissPath, std::string closestHitPath);
    void CreatePipeline(VkDevice device, std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, uint32_t maxRecursionDepth);
    void Cleanup(VkDevice device, VmaAllocator allocator);
    void DestroyShaders(VkDevice device);
    void CreateShaderBindingTable(VkDevice device, VmaAllocator allocator, VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties);

    uint64_t get_buffer_device_address(VkDevice device, VkBuffer buffer);
    uint32_t alignedSize(uint32_t value, uint32_t alignment);*/

    // This is terrible. You are reloading these function pointers for each ray tracer instance
    PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
    PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
    PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
    PFN_vkDebugMarkerSetObjectTagEXT pfnDebugMarkerSetObjectTag;
    PFN_vkDebugMarkerSetObjectNameEXT pfnDebugMarkerSetObjectName;
    PFN_vkCmdDebugMarkerBeginEXT pfnCmdDebugMarkerBegin;
    PFN_vkCmdDebugMarkerEndEXT pfnCmdDebugMarkerEnd;
    PFN_vkCmdDebugMarkerInsertEXT pfnCmdDebugMarkerInsert;
    PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;

    void DestroyShaders(VkDevice device) {
        if (shaderStack.rayGen) {
            vkDestroyShaderModule(device, shaderStack.rayGen, nullptr);
            shaderStack.rayGen = VK_NULL_HANDLE;
        }
        if (shaderStack.rayMiss) {
            vkDestroyShaderModule(device, shaderStack.rayMiss, nullptr);
            shaderStack.rayGen = VK_NULL_HANDLE;
        }
        if (shaderStack.rayshadowMiss) {
            vkDestroyShaderModule(device, shaderStack.rayshadowMiss, nullptr);
            shaderStack.rayGen = VK_NULL_HANDLE;
        }
        if (shaderStack.closestHit) {
            vkDestroyShaderModule(device, shaderStack.closestHit, nullptr);
            shaderStack.rayGen = VK_NULL_HANDLE;
        }
    }

    void Cleanup(VkDevice device, VmaAllocator allocator) {
        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

        pipeline = VK_NULL_HANDLE;
        pipelineLayout = VK_NULL_HANDLE;

        vmaDestroyBuffer(allocator, raygenShaderBindingTable._buffer, raygenShaderBindingTable._allocation);
        vmaDestroyBuffer(allocator, missShaderBindingTable._buffer, missShaderBindingTable._allocation);
        vmaDestroyBuffer(allocator, hitShaderBindingTable._buffer, hitShaderBindingTable._allocation);
    }

    bool LoadShaders(VkDevice device, std::string rayGenPath, std::string missPath, std::string shadowMissPath, std::string closestHitPath) {

        vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));
        vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));
        vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkBuildAccelerationStructuresKHR"));
        vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
        vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
        vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
        vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR"));
        vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR"));
        vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR"));
        vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));

        ShaderStack tempShaderStack;
        if (!VulkanShaderUtil::LoadShader(device, rayGenPath, VK_SHADER_STAGE_RAYGEN_BIT_KHR, &tempShaderStack.rayGen) ||
            !VulkanShaderUtil::LoadShader(device, missPath, VK_SHADER_STAGE_MISS_BIT_KHR, &tempShaderStack.rayMiss) ||
            !VulkanShaderUtil::LoadShader(device, closestHitPath, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, &tempShaderStack.closestHit) ||
            !VulkanShaderUtil::LoadShader(device, shadowMissPath, VK_SHADER_STAGE_MISS_BIT_KHR, &tempShaderStack.rayshadowMiss)) {
            return false;
        }
        shaderStack = tempShaderStack;
        shaderGroups.clear();
        shaderStages.clear();

        // Ray generation group
        {
            VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
            shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            shaderGroup.generalShader = 0;
            shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
            shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
            shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
            shaderGroups.push_back(shaderGroup);

            VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = {};
            pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            pipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
            pipelineShaderStageCreateInfo.module = shaderStack.rayGen;
            pipelineShaderStageCreateInfo.pNext = nullptr;
            pipelineShaderStageCreateInfo.pName = "main";
            shaderStages.emplace_back(pipelineShaderStageCreateInfo);
        }

        // Miss group
        {
            VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
            shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            shaderGroup.generalShader = 1;
            shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
            shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
            shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
            shaderGroups.push_back(shaderGroup);

            VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = {};
            pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            pipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
            pipelineShaderStageCreateInfo.module = shaderStack.rayMiss;
            pipelineShaderStageCreateInfo.pNext = nullptr;
            pipelineShaderStageCreateInfo.pName = "main";
            shaderStages.emplace_back(pipelineShaderStageCreateInfo);

            // Second shader for shadows
            shaderGroup.generalShader = 2;
            shaderGroups.push_back(shaderGroup);

            VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo2 = {};
            pipelineShaderStageCreateInfo2.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            pipelineShaderStageCreateInfo2.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
            pipelineShaderStageCreateInfo2.module = shaderStack.rayshadowMiss;
            pipelineShaderStageCreateInfo2.pNext = nullptr;
            pipelineShaderStageCreateInfo2.pName = "main";
            shaderStages.emplace_back(pipelineShaderStageCreateInfo2);

        }

        // Closest hit group
        {
            VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
            shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
            shaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
            shaderGroup.closestHitShader = 3;
            shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
            shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
            shaderGroups.push_back(shaderGroup);

            VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = {};
            pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            pipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
            pipelineShaderStageCreateInfo.module = shaderStack.closestHit;
            pipelineShaderStageCreateInfo.pNext = nullptr;
            pipelineShaderStageCreateInfo.pName = "main";
            shaderStages.emplace_back(pipelineShaderStageCreateInfo);
        }
        return true;
    }

    void CreateRaytracingPipeline(VkDevice device, std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, uint32_t maxRecursionDepth) {

        if (pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, pipeline, nullptr);
        }
        if (pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        }

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = descriptorSetLayouts.size();
        pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
        VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));
        VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCI{};
        rayTracingPipelineCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
        rayTracingPipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
        rayTracingPipelineCI.pStages = shaderStages.data();
        rayTracingPipelineCI.groupCount = static_cast<uint32_t>(shaderGroups.size());
        rayTracingPipelineCI.pGroups = shaderGroups.data();
        rayTracingPipelineCI.maxPipelineRayRecursionDepth = maxRecursionDepth;
        rayTracingPipelineCI.layout = pipelineLayout;
        vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineCI, nullptr, &pipeline);

        // This wasn't a problem before, but is now apparently.
        // A validation error showed up at some point complaining that the ray tracing pipeline didn't support dynamic state,
        // yet there were dynamic state commands in the command buffer.
        //VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};
        //VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        //dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        //dynamicStateInfo.dynamicStateCount = 2;
        //dynamicStateInfo.pDynamicStates = dynamicStates;
        //rayTracingPipelineCI.pDynamicState = &dynamicStateInfo;
        // actrually was my fault.... maybe...

        std::cout << "Created raytracing pipeline\n";
        std::cout << "pipeline: " << pipeline << "\n";
        std::cout << "pipelineLayout: " << pipelineLayout << "\n";        
    }

    uint64_t get_buffer_device_address(VkDevice device, VkBuffer buffer)
    {
        VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
        bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        bufferDeviceAI.buffer = buffer;
        return vkGetBufferDeviceAddressKHR(device, &bufferDeviceAI);
    }


    uint32_t alignedSize(uint32_t value, uint32_t alignment) {
        return (value + alignment - 1) & ~(alignment - 1);

    }
    void CreateShaderBindingTable(VkDevice device, VmaAllocator allocator, VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties)
    {
        const uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
        const uint32_t handleSizeAligned = alignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);
        const uint32_t groupCount = static_cast<uint32_t>(shaderGroups.size());
        const uint32_t sbtSize = groupCount * handleSizeAligned;

        std::vector<uint8_t> shaderHandleStorage(sbtSize);
        VK_CHECK(vkGetRayTracingShaderGroupHandlesKHR(device, pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data()));

        const VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        const VkMemoryPropertyFlags memoryUsageFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        VkBufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.usage = bufferUsageFlags;// VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        //createInfo.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT; // this might not actually be the solution, you added it on a whim kinda
        createInfo.size = handleSize;
        createInfo.pNext = nullptr;

        VmaAllocationCreateInfo vmaallocInfo = {};
        vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        vmaallocInfo.preferredFlags = memoryUsageFlags;

        VK_CHECK(vmaCreateBuffer(allocator, &createInfo, &vmaallocInfo, &raygenShaderBindingTable._buffer, &raygenShaderBindingTable._allocation, nullptr));
        VK_CHECK(vmaCreateBuffer(allocator, &createInfo, &vmaallocInfo, &missShaderBindingTable._buffer, &missShaderBindingTable._allocation, nullptr));
        VK_CHECK(vmaCreateBuffer(allocator, &createInfo, &vmaallocInfo, &hitShaderBindingTable._buffer, &hitShaderBindingTable._allocation, nullptr));

        // Copy handles
        void* data;
        vmaMapMemory(allocator, raygenShaderBindingTable._allocation, &data);
        memcpy(data, shaderHandleStorage.data(), handleSize);
        vmaUnmapMemory(allocator, raygenShaderBindingTable._allocation);

        vmaMapMemory(allocator, missShaderBindingTable._allocation, &data);
        memcpy(data, shaderHandleStorage.data() + handleSizeAligned, handleSize * 2);
        vmaUnmapMemory(allocator, missShaderBindingTable._allocation);

        vmaMapMemory(allocator, hitShaderBindingTable._allocation, &data);
        memcpy(data, shaderHandleStorage.data() + handleSizeAligned * 3, handleSize);
        vmaUnmapMemory(allocator, hitShaderBindingTable._allocation);

        //Setup the buffer regions pointing to the shaders in our shader binding table
        raygenShaderSbtEntry.deviceAddress = get_buffer_device_address(device, raygenShaderBindingTable._buffer);
        raygenShaderSbtEntry.stride = handleSizeAligned;
        raygenShaderSbtEntry.size = handleSizeAligned;

        missShaderSbtEntry.deviceAddress = get_buffer_device_address(device, missShaderBindingTable._buffer);
        missShaderSbtEntry.stride = handleSizeAligned;
        missShaderSbtEntry.size = handleSizeAligned;

        hitShaderSbtEntry.deviceAddress = get_buffer_device_address(device, hitShaderBindingTable._buffer);
        hitShaderSbtEntry.stride = handleSizeAligned;
        hitShaderSbtEntry.size = handleSizeAligned;
    }
};
