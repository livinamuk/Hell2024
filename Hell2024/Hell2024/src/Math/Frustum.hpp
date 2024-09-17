#pragma once
#include "HellCommon.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include "RendererCommon.h"

// a 3-bit encoding representing a box corner
using BoxCorner = int;

struct Plane {
    glm::vec3 normal;
    float offset;
    float normalLength;
};

struct Frustum {

    void Update(const glm::mat4& projectionView) {
        glm::vec4 l = glm::row(projectionView, 3) + glm::row(projectionView, 0);
        glm::vec4 r = glm::row(projectionView, 3) - glm::row(projectionView, 0);
        glm::vec4 b = glm::row(projectionView, 3) + glm::row(projectionView, 1);
        glm::vec4 t = glm::row(projectionView, 3) - glm::row(projectionView, 1);
        glm::vec4 n = glm::row(projectionView, 3) + glm::row(projectionView, 2);
        glm::vec4 f = glm::row(projectionView, 3) - glm::row(projectionView, 2);
        m_planes[0] = { -glm::vec3(l), -l.w }; // Left
        m_planes[1] = { -glm::vec3(r), -r.w }; // Right
        m_planes[2] = { -glm::vec3(b), -b.w }; // Bottom
        m_planes[3] = { -glm::vec3(t), -t.w }; // Top
        m_planes[4] = { -glm::vec3(n), -n.w }; // Near
        m_planes[5] = { -glm::vec3(f), -f.w }; // Far
        for (int i = 0; i < 6; i++) {
            float normalLength = glm::length(m_planes[i].normal);
            m_planes[i].normal = glm::normalize(m_planes[i].normal);
            m_planes[i].offset = m_planes[i].offset / normalLength;
            m_minBoxCorners[i] = CreateMinBoxCorner(m_planes[i]);
        }
    }

    bool IntersectsAABB(const AABB& aabb) {
        for (int i = 0; i < 6; ++i) {
            bool all_outside = true;
            for (int j = 0; j < 8; ++j) {
                glm::vec3 corner = CreateBoxCorner(aabb, j);
                if (SignedDistance(corner, m_planes[i]) <= 0.0f) {
                    all_outside = false;
                    break;
                }
            }
            if (all_outside) {
                return false;
            }
        }
        return true;
    }

    bool IntersectsAABBFast(const AABB& aabb) {
        for (int i = 0; i < 6; ++i) {
            glm::vec3 min_corner = CreateBoxCorner(aabb, m_minBoxCorners[i]);
            if (SignedDistance(min_corner, m_planes[i]) > 0.0f)
                return false;
        }
        return true;
    }

    bool IntersectsAABBFast(const RenderItem3D& renderItem) {
        for (int i = 0; i < 6; ++i) {
            glm::vec3 min_corner = CreateBoxCorner(renderItem, m_minBoxCorners[i]);
            if (SignedDistance(min_corner, m_planes[i]) > 0.0f)
                return false;
        }
        return true;
    }

    bool IntersectsSphere(const Sphere& sphere) const {
        for (int i = 0; i < 6; ++i) {
            float distance = SignedDistance(sphere.origin, m_planes[i]);
            if (distance > sphere.radius) {
                return false;
            }
        }
        return true;
    }

private:

    Plane m_planes[6];
    BoxCorner m_minBoxCorners[6];

    bool IsBitSet(int x, int bit) {
        return (x >> bit) & 1;
    }

    glm::vec3 CreateBoxCorner(const AABB& aabb, BoxCorner corner) {
        return glm::vec3(
            IsBitSet(corner, 0) ? aabb.boundsMin.x : aabb.boundsMax.x,
            IsBitSet(corner, 1) ? aabb.boundsMin.y : aabb.boundsMax.y,
            IsBitSet(corner, 2) ? aabb.boundsMin.z : aabb.boundsMax.z
        );
    }

    glm::vec3 CreateBoxCorner(const RenderItem3D& renderItem, BoxCorner corner) {
        return glm::vec3(
            IsBitSet(corner, 0) ? renderItem.aabbMin.x : renderItem.aabbMax.x,
            IsBitSet(corner, 1) ? renderItem.aabbMin.y : renderItem.aabbMax.y,
            IsBitSet(corner, 2) ? renderItem.aabbMin.z : renderItem.aabbMax.z
        );
    }

    float SignedDistance(const glm::vec3& point, const Plane& plane) const {
        return (glm::dot(plane.normal, point) + plane.offset);
    }

    BoxCorner CreateMinBoxCorner(const Plane& plane) {
        AABB aabb{ glm::vec3(0,0,0), glm::vec3(1,1,1) };
        std::pair<float, BoxCorner> distance_corner_pairs[8];
        for (BoxCorner i = 0; i < 8; ++i) {
            distance_corner_pairs[i] = { SignedDistance(CreateBoxCorner(aabb, i), plane), i };
        }
        return std::min_element(begin(distance_corner_pairs), end(distance_corner_pairs))->second;
    }
};

