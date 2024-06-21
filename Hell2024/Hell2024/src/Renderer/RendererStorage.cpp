#include "RendererStorage.h"
#include <iostream>

namespace RendererStorage {

    std::vector<VertexBuffer> gSkinnedMeshVertexBuffers;

    int CreateSkinnedVertexBuffer() {
        gSkinnedMeshVertexBuffers.emplace_back();
        return (int)gSkinnedMeshVertexBuffers.size() - 1;
    }

    VertexBuffer* GetSkinnedVertexBufferByIndex(int index) {
        if (index >= 0 && index < gSkinnedMeshVertexBuffers.size()) {
            return &gSkinnedMeshVertexBuffers[index];
        }
        else {
            std::cout << "GetSkinnedVertexBufferByIndex() called with out of range index " << index << ". Size is " << gSkinnedMeshVertexBuffers.size() << "!\n";
        }
    }
}