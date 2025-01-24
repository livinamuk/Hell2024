#pragma once
#include "Types/VertexBuffer.h"

namespace RendererStorage {

    int CreateSkinnedVertexBuffer();
    VertexBuffer* GetSkinnedVertexBufferByIndex(int index);
}