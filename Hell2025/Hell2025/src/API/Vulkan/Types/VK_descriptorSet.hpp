#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <iostream>
#include <string>

struct DescriptorSet {

    std::vector<VkDescriptorSetLayoutBinding> bindings;
    VkDescriptorSetLayout layout;
    VkDescriptorSet handle;
    std::string debugName;

    void SetDebugName(const std::string& name) {
        debugName = name;
    }

    void AddBinding(VkDescriptorType type, uint32_t binding, uint32_t descriptorCount, VkShaderStageFlags stageFlags) {
        VkDescriptorSetLayoutBinding setbind = {};
        setbind.binding = binding;
        setbind.descriptorCount = descriptorCount;
        setbind.descriptorType = type;
        setbind.pImmutableSamplers = nullptr;
        setbind.stageFlags = stageFlags;
        bindings.push_back(setbind);
    }

    void BuildSetLayout(VkDevice device) {
        VkDescriptorSetLayoutCreateInfo setinfo = {};
        setinfo.bindingCount = (uint32_t)bindings.size();
        setinfo.flags = 0;
        setinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        setinfo.pBindings = bindings.data();
        setinfo.pNext = nullptr;
        VkResult res = vkCreateDescriptorSetLayout(device, &setinfo, nullptr, &layout);
        if (res != VK_SUCCESS) {
            std::cout << "Error creating desc set layout\n";
        }
    }

    void AllocateSet(VkDevice device, VkDescriptorPool descriptorPool) {
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.pNext = nullptr;
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout;
        vkAllocateDescriptorSets(device, &allocInfo, &handle);
    }

    void Update(VkDevice device, uint32_t binding, uint32_t descriptorCount, VkDescriptorType type, VkBuffer buffer) {
        if (buffer == VK_NULL_HANDLE) {
            std::cout << "WARNING!!! PROBABLY ERROR! You called Update on a descriptor set but passed in a null buffer, name: '" << debugName << "'\n";
        }
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = buffer;
        bufferInfo.range = VK_WHOLE_SIZE;
        bufferInfo.offset = 0;
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = handle;
        write.descriptorType = type;
        write.dstBinding = binding;
        write.pBufferInfo = &bufferInfo;
        write.descriptorCount = descriptorCount;
        vkUpdateDescriptorSets(device, 1, &write, 0, VK_NULL_HANDLE);
    }

    void Update(VkDevice device, uint32_t binding, uint32_t descriptorCount, VkDescriptorType type, VkDescriptorImageInfo* imageInfo) {
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = handle;
        write.descriptorType = type;
        write.dstBinding = binding;
        write.descriptorCount = descriptorCount;
        write.pImageInfo = imageInfo;
        vkUpdateDescriptorSets(device, 1, &write, 0, VK_NULL_HANDLE);
    }

    void Update(VkDevice device, uint32_t binding, uint32_t descriptorCount, VkDescriptorType type, VkAccelerationStructureKHR* accelerationStructure) {
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = handle;
        write.dstBinding = binding;
        write.descriptorCount = descriptorCount;
        write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        VkWriteDescriptorSetAccelerationStructureKHR accelerationStructreInfo{};
        accelerationStructreInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        accelerationStructreInfo.accelerationStructureCount = 1;
        accelerationStructreInfo.pAccelerationStructures = accelerationStructure;
        write.pNext = &accelerationStructreInfo;
        vkUpdateDescriptorSets(device, 1, &write, 0, VK_NULL_HANDLE);
    }

    void Destroy(VkDevice device) {
        vkDestroyDescriptorSetLayout(device, layout, nullptr);
    }

};