#include "VK_mesh.h"
#include <iostream>
#include <unordered_map>
#include "../VK_Util.hpp"
#include "../VK_assetManager.h"

void VulkanMesh::Draw(VkCommandBuffer commandBuffer, uint32_t firstInstance) {
    if (_vertexCount <= 0)
        return;

    VkDeviceSize offset = 0;
    if (_indexCount > 0) {
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_vertexBuffer._buffer, &offset);
        vkCmdBindIndexBuffer(commandBuffer, _indexBuffer._buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(_indexCount), 1, 0, 0, firstInstance);
    }
    else {
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_vertexBuffer._buffer, &offset);
        vkCmdDraw(commandBuffer, _vertexCount, 1, 0, firstInstance);
    }
}

VulkanModel::VulkanModel() {
    // intentionally blank
}

void VulkanModel::Draw(VkCommandBuffer commandBuffer, uint32_t firstInstance) {
    for (int i = 0; i < _meshIndices.size(); i++)
        VulkanAssetManager::GetMesh(_meshIndices[i])->Draw(commandBuffer, firstInstance);
}