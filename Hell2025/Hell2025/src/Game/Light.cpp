#include "Light.h"
#include "../Game/Scene.h"
#include "../Editor/CSG.h"
#include "../Math/Raycasting.hpp"
#include "../Renderer/Raytracing/Raytracing.h"
#include "../Renderer/GlobalIllumination.h"
#include "../Util.hpp"

void Light::FindVisibleCloudPoints() {

    return;

    std::vector<CSGVertex>& vertices = CSG::GetVertices();
    std::vector<uint32_t>& indices = CSG::GetIndices();
    auto& pointCloud = GlobalIllumination::GetPointCloud();
    std::vector<BLASInstance> blasInstaces = Raytracing::GetBLASInstances(0);
    std::vector<CSGObject>& csgObjects = CSG::GetCSGObjects();

    visibleCloudPointIndices.clear();


    for (int j = 0; j < GlobalIllumination::GetPointCloud().size(); j++) {

        CloudPoint& point = pointCloud[j];

        float distanceToPointSquared = Util::DistanceSquared(this->position, point.position);

        // Is the point out of the lights influence? then bail
        if (distanceToPointSquared > radius * radius) {
            continue;
        }

        bool hasLineOfSight = true;

        for (CSGObject& csgObject : csgObjects) {

            for (int i = csgObject.m_baseIndex; i < csgObject.m_baseIndex + csgObject.m_indexCount; i += 3) {

                uint32_t idx0 = indices[i + 0 + csgObject.m_baseIndex] + csgObject.m_baseVertex;
                uint32_t idx1 = indices[i + 1 + csgObject.m_baseIndex] + csgObject.m_baseVertex;
                uint32_t idx2 = indices[i + 2 + csgObject.m_baseIndex] + csgObject.m_baseVertex;
                CSGVertex v0 = vertices[idx0];
                CSGVertex v1 = vertices[idx1];
                CSGVertex v2 = vertices[idx2];

                glm::vec3 rayOrigin = point.position + point.normal * glm::vec3(0.01);
                glm::vec3 rayDir = glm::normalize(position - point.position);

                IntersectionResult rayResult = Math::RayTriangleIntersectTest(rayOrigin, rayDir, v0.position, v1.position, v2.position);

                // Has line of sight
                if (rayResult.hitFound) {
                    hasLineOfSight = false;
                    break;
                }
            }
        }

        if (hasLineOfSight) {
            visibleCloudPointIndices.push_back(j);
        }
    }

    std::cout << "Point cloud size: " << GlobalIllumination::GetPointCloud().size() << "\n";
    std::cout << "visibleCloudPointIndices.size(): " << visibleCloudPointIndices.size() << "\n";
}