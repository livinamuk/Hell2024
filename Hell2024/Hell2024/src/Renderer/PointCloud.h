#include "RendererCommon.h"

struct CloudPoint {
    glm::vec4 position = glm::vec4(0);
    glm::vec4 normal = glm::vec4(0);
};

namespace PointCloud {

    std::vector<CloudPoint>& GetCloud();
    void Create();
}