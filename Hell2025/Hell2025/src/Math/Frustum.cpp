#include "Frustum.h"
#include "../Util.hpp"

void Frustum::Update(const glm::mat4& projectionView) {

    // Left clipping plane
    m_planes[0].normal.x = projectionView[0][3] + projectionView[0][0];
    m_planes[0].normal.y = projectionView[1][3] + projectionView[1][0];
    m_planes[0].normal.z = projectionView[2][3] + projectionView[2][0];
    m_planes[0].offset = projectionView[3][3] + projectionView[3][0];

    // Right clipping plane
    m_planes[1].normal.x = projectionView[0][3] - projectionView[0][0];
    m_planes[1].normal.y = projectionView[1][3] - projectionView[1][0];
    m_planes[1].normal.z = projectionView[2][3] - projectionView[2][0];
    m_planes[1].offset = projectionView[3][3] - projectionView[3][0];

    // Top clipping plane
    m_planes[2].normal.x = projectionView[0][3] - projectionView[0][1];
    m_planes[2].normal.y = projectionView[1][3] - projectionView[1][1];
    m_planes[2].normal.z = projectionView[2][3] - projectionView[2][1];
    m_planes[2].offset = projectionView[3][3] - projectionView[3][1];

    // Bottom clipping plane
    m_planes[3].normal.x = projectionView[0][3] + projectionView[0][1];
    m_planes[3].normal.y = projectionView[1][3] + projectionView[1][1];
    m_planes[3].normal.z = projectionView[2][3] + projectionView[2][1];
    m_planes[3].offset = projectionView[3][3] + projectionView[3][1];

    // Near clipping plane
    m_planes[4].normal.x = projectionView[0][3] + projectionView[0][2];
    m_planes[4].normal.y = projectionView[1][3] + projectionView[1][2];
    m_planes[4].normal.z = projectionView[2][3] + projectionView[2][2];
    m_planes[4].offset = projectionView[3][3] + projectionView[3][2];

    // Far clipping plane
    m_planes[5].normal.x = projectionView[0][3] - projectionView[0][2];
    m_planes[5].normal.y = projectionView[1][3] - projectionView[1][2];
    m_planes[5].normal.z = projectionView[2][3] - projectionView[2][2];
    m_planes[5].offset = projectionView[3][3] - projectionView[3][2];

    // Normalize planes
    for (int i = 0; i < 6; i++) {
        float magnitude = glm::length(m_planes[i].normal);
        m_planes[i].normal /= magnitude;
        m_planes[i].offset /= magnitude;
    }
}

void Frustum::UpdateFromTile(const glm::mat4& viewMatrix, float fov, float nearPlane, float farPlane, int x1, int y1, int x2, int y2, int viewportWidth, int viewportHeight) {
    float aspectRatio = (float)viewportWidth / (float)viewportHeight;
    float nearHeight = 2.0f * tan(fov / 2.0f) * nearPlane;
    float nearWidth = nearHeight * aspectRatio;
    float leftNDC = static_cast<float>(x1) / (float)viewportWidth;
    float rightNDC = static_cast<float>(x2) / (float)viewportWidth;
    float bottomNDC = static_cast<float>(y1) / (float)viewportHeight;
    float topNDC = static_cast<float>(y2) / (float)viewportHeight;
    float left = (leftNDC - 0.5f) * nearWidth;
    float right = (rightNDC - 0.5f) * nearWidth;
    float bottom = (bottomNDC - 0.5f) * nearHeight;
    float top = (topNDC - 0.5f) * nearHeight;
    glm::mat4 tileProj = glm::frustum(left, right, bottom, top, nearPlane, farPlane);
    glm::mat4 viewProj = tileProj * viewMatrix;
    Update(viewProj);
}

bool Frustum::IntersectsAABB(const AABB& aabb) {
    glm::vec3 aabbCorners[8] = {
        glm::vec3(aabb.boundsMin.x, aabb.boundsMin.y, aabb.boundsMin.z), // Near-bottom-left
        glm::vec3(aabb.boundsMax.x, aabb.boundsMin.y, aabb.boundsMin.z), // Near-bottom-right
        glm::vec3(aabb.boundsMin.x, aabb.boundsMax.y, aabb.boundsMin.z), // Near-top-left
        glm::vec3(aabb.boundsMax.x, aabb.boundsMax.y, aabb.boundsMin.z), // Near-top-right
        glm::vec3(aabb.boundsMin.x, aabb.boundsMin.y, aabb.boundsMax.z), // Far-bottom-left
        glm::vec3(aabb.boundsMax.x, aabb.boundsMin.y, aabb.boundsMax.z), // Far-bottom-right
        glm::vec3(aabb.boundsMin.x, aabb.boundsMax.y, aabb.boundsMax.z), // Far-top-left
        glm::vec3(aabb.boundsMax.x, aabb.boundsMax.y, aabb.boundsMax.z)  // Far-top-right
    };
    for (int i = 0; i < 6; ++i) {
        int pointsOutside = 0;
        for (int j = 0; j < 8; ++j) {
            if (SignedDistance(aabbCorners[j], m_planes[i]) < 0.0f) {
                pointsOutside++;
            }
        }
        if (pointsOutside == 8) {
            return false;
        }
    }
    return true;
}


bool Frustum::IntersectsAABB(const RenderItem3D& renderItem) {
    glm::vec3 aabbCorners[8] = {
        glm::vec3(renderItem.aabbMin.x, renderItem.aabbMin.y, renderItem.aabbMax.z), // Near-bottom-left
        glm::vec3(renderItem.aabbMax.x, renderItem.aabbMin.y, renderItem.aabbMin.z), // Near-bottom-right
        glm::vec3(renderItem.aabbMin.x, renderItem.aabbMax.y, renderItem.aabbMin.z), // Near-top-left
        glm::vec3(renderItem.aabbMax.x, renderItem.aabbMax.y, renderItem.aabbMin.z), // Near-top-right
        glm::vec3(renderItem.aabbMin.x, renderItem.aabbMin.y, renderItem.aabbMax.z), // Far-bottom-left
        glm::vec3(renderItem.aabbMax.x, renderItem.aabbMin.y, renderItem.aabbMax.z), // Far-bottom-right
        glm::vec3(renderItem.aabbMin.x, renderItem.aabbMax.y, renderItem.aabbMax.z), // Far-top-left
        glm::vec3(renderItem.aabbMax.x, renderItem.aabbMax.y, renderItem.aabbMax.z)  // Far-top-right
    };
    for (int i = 0; i < 6; ++i) {
        int pointsOutside = 0;
        for (int j = 0; j < 8; ++j) {
            if (SignedDistance(aabbCorners[j], m_planes[i]) < 0.0f) {
                pointsOutside++;
            }
        }
        if (pointsOutside == 8) {
            return false;
        }
    }
    return true;
}

bool Frustum::IntersectsAABBFast(const AABB& aabb) {
    for (int i = 0; i < 6; ++i) {
        glm::vec3 min_corner = glm::vec3(
            m_planes[i].normal.x > 0 ? aabb.boundsMax.x : aabb.boundsMin.x,
            m_planes[i].normal.y > 0 ? aabb.boundsMax.y : aabb.boundsMin.y,
            m_planes[i].normal.z > 0 ? aabb.boundsMax.z : aabb.boundsMin.z
        );
        if (SignedDistance(min_corner, m_planes[i]) <= 0.0f) {
            return false; 
        }
    }
    return true;
}

bool Frustum::IntersectsAABBFast(const RenderItem3D& renderItem) {
    for (int i = 0; i < 6; ++i) {
        glm::vec3 min_corner = glm::vec3(
            m_planes[i].normal.x > 0 ? renderItem.aabbMax.x : renderItem.aabbMin.x,
            m_planes[i].normal.y > 0 ? renderItem.aabbMax.y : renderItem.aabbMin.y,
            m_planes[i].normal.z > 0 ? renderItem.aabbMax.z : renderItem.aabbMin.z
        );
        if (SignedDistance(min_corner, m_planes[i]) <= 0.0f) {
            return false;
        }
    }
    return true;
}

bool Frustum::IntersectsSphere(const Sphere& sphere) {
    for (int i = 0; i < 6; ++i) {
        float distance = SignedDistance(sphere.origin, m_planes[i]);
        if (distance + sphere.radius < 0) {
            return false;
        }
    }
    return true;
}

bool Frustum::IntersectsPoint(const glm::vec3 point) {
    bool insideFrustum = true;
    for (int i = 0; i < 6; i++) {
        float distance = SignedDistance(point, m_planes[i]);
        if (distance < 0) {
            return false;
        }
    }
    return true;
}

std::vector<glm::vec3> Frustum::GetFrustumCorners() {
    std::vector<glm::vec3> corners(8);
    corners[0] = IntersectPlanes(m_planes[0].normal, m_planes[0].offset, m_planes[2].normal, m_planes[2].offset, m_planes[4].normal, m_planes[4].offset); // Near bottom-left
    corners[1] = IntersectPlanes(m_planes[1].normal, m_planes[1].offset, m_planes[2].normal, m_planes[2].offset, m_planes[4].normal, m_planes[4].offset); // Near bottom-right
    corners[2] = IntersectPlanes(m_planes[0].normal, m_planes[0].offset, m_planes[3].normal, m_planes[3].offset, m_planes[4].normal, m_planes[4].offset); // Near top-left
    corners[3] = IntersectPlanes(m_planes[1].normal, m_planes[1].offset, m_planes[3].normal, m_planes[3].offset, m_planes[4].normal, m_planes[4].offset); // Near top-right  
    corners[4] = IntersectPlanes(m_planes[0].normal, m_planes[0].offset, m_planes[2].normal, m_planes[2].offset, m_planes[5].normal, m_planes[5].offset); // Far bottom-left
    corners[5] = IntersectPlanes(m_planes[1].normal, m_planes[1].offset, m_planes[2].normal, m_planes[2].offset, m_planes[5].normal, m_planes[5].offset); // Far bottom-right
    corners[6] = IntersectPlanes(m_planes[0].normal, m_planes[0].offset, m_planes[3].normal, m_planes[3].offset, m_planes[5].normal, m_planes[5].offset); // Far top-left
    corners[7] = IntersectPlanes(m_planes[1].normal, m_planes[1].offset, m_planes[3].normal, m_planes[3].offset, m_planes[5].normal, m_planes[5].offset); // Far top-right
    return corners;
}

Plane Frustum::CreatePlane(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3) {
    Plane plane;
    plane.normal = glm::normalize(glm::cross(p2 - p1, p3 - p1));
    plane.offset = -glm::dot(plane.normal, p1);
    return plane;
}

float Frustum::SignedDistance(const glm::vec3& point, const Plane& plane) const {
    return glm::dot(plane.normal, point) + plane.offset;
}

glm::vec3 Frustum::IntersectPlanes(const glm::vec3& n1, float d1, const glm::vec3& n2, float d2, const glm::vec3& n3, float d3) {
    glm::vec3 crossN2N3 = glm::cross(n2, n3);
    float denom = glm::dot(n1, crossN2N3);
    if (std::fabs(denom) < 1e-6f) {
        return glm::vec3(0.0f);
    }
    glm::vec3 result = -(d1 * crossN2N3 + d2 * glm::cross(n3, n1) + d3 * glm::cross(n1, n2)) / denom;
    return result;
}