#include "VK_types.h"

//////////////////////////////////
//                              //
//      Hell Descriptor Set     //

void HellDescriptorSet::AddBinding(VkDescriptorType type, uint32_t binding, uint32_t descriptorCount, VkShaderStageFlags stageFlags) {
    VkDescriptorSetLayoutBinding setbind = {};
    setbind.binding = binding;
    setbind.descriptorCount = descriptorCount;
    setbind.descriptorType = type;
    setbind.pImmutableSamplers = nullptr;
    setbind.stageFlags = stageFlags;
    bindings.push_back(setbind);
}

void HellDescriptorSet::BuildSetLayout(VkDevice device) {
    VkDescriptorSetLayoutCreateInfo setinfo = {};
    setinfo.bindingCount = bindings.size();
    setinfo.flags = 0;
    setinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setinfo.pBindings = bindings.data();
    setinfo.pNext = nullptr;
    VkResult res = vkCreateDescriptorSetLayout(device, &setinfo, nullptr, &layout);

    if (res != VK_SUCCESS) {
        std::cout << "Error creating desc set layout\n";
    }
}

void HellDescriptorSet::AllocateSet(VkDevice device, VkDescriptorPool descriptorPool) {
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.pNext = nullptr;
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;
    vkAllocateDescriptorSets(device, &allocInfo, &handle);
}

void HellDescriptorSet::Update(VkDevice device, uint32_t binding, uint32_t descriptorCount, VkDescriptorType type, VkBuffer buffer)
{
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

void HellDescriptorSet::Update(VkDevice device, uint32_t binding, uint32_t descriptorCount, VkDescriptorType type, VkDescriptorImageInfo* imageInfo)
{
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = handle;
    write.descriptorType = type;
    write.dstBinding = binding;
    write.descriptorCount = descriptorCount;
    write.pImageInfo = imageInfo;
    vkUpdateDescriptorSets(device, 1, &write, 0, VK_NULL_HANDLE);
}

void HellDescriptorSet::Update(VkDevice device, uint32_t binding, uint32_t descriptorCount, VkDescriptorType type, VkAccelerationStructureKHR* accelerationStructure)
{
    // Acceleration structure	
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

void HellDescriptorSet::Destroy(VkDevice device) {
    vkDestroyDescriptorSetLayout(device, layout, nullptr);
}


//////////////////////////
//                      //
//      Hell Buffer     //

void HellBuffer::Create(VmaAllocator allocator, unsigned int srcSize, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryUsageFlags) {
    VkBufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.usage = bufferUsageFlags;
    createInfo.size = srcSize;
    createInfo.pNext = nullptr;
    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    vmaallocInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    vmaCreateBuffer(allocator, &createInfo, &vmaallocInfo, &buffer, &allocation, nullptr);

    VkDebugUtilsObjectNameInfoEXT nameInfo = {};
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
    nameInfo.objectHandle = (uint64_t)buffer;
    nameInfo.pObjectName = "Misc HellBuffer";
    size = srcSize;
}

void HellBuffer::Map(VmaAllocator allocator, void* srcData) {
    void* data = nullptr;
    vmaMapMemory(allocator, allocation, &data);
    memcpy(data, srcData, size);
    vmaUnmapMemory(allocator, allocation);
}

void HellBuffer::MapRange(VmaAllocator allocator, void* srcData, size_t size) {
    void* data = nullptr;
    vmaMapMemory(allocator, allocation, &data);
    memcpy(data, srcData, size);
    vmaUnmapMemory(allocator, allocation);
}

void HellBuffer::Destroy(VmaAllocator allocator) {
    vmaDestroyBuffer(allocator, buffer, allocation);
}