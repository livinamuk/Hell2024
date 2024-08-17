#pragma once
#include "../Common.h"
#include "../Renderer/RendererCommon.h"

struct Plane {

    glm::vec3 normal = { 0.0f, 1.0f, 0.0f };
    float distance = 0.0f;

    Plane() = default;
    Plane(const glm::vec3& p1, const glm::vec3& _normal) {
        normal = glm::normalize(_normal);
        distance = glm::dot(normal, p1);
    }
    Plane(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3) {
        normal = glm::normalize(glm::cross(p2 - p1, p3 - p1));
        distance = glm::dot(normal, p1);
    }

    float GetSignedDistanceToPlane(const glm::vec3& point) const {
        return glm::dot(normal, point) - distance;
    }

    /*
    glm::vec4 PlaneFromCorners(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) {
        glm::vec3 normal = glm::cross(p2 - p1, p3 - p1);
        glm::vec4 plane;
        SetPlane(plane, p1, normal);
        return plane;
    }*/
};


struct Frustum {

    Plane topFace;
    Plane bottomFace;
    Plane rightFace;
    Plane leftFace;
    Plane farFace;
    Plane nearFace;




    void Update(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix) {

        std::vector<glm::vec3> corners = GetFrustumCorners(projectionMatrix, viewMatrix);

        return;

        // Define the planes using the corners
        nearFace = Plane(corners[4], corners[5], corners[7]);    // near = ntl, ntr, nbr
        farFace = Plane(corners[1], corners[0], corners[2]);     // far = ftl, ftr, fbl
        leftFace = Plane(corners[0], corners[4], corners[6]);    // left = ftl, ntl, nbl
        rightFace = Plane(corners[5], corners[1], corners[7]);   // right = ntr, ftr, nbr
        topFace = Plane(corners[0], corners[1], corners[4]);     // top = ftl, ftr, ntl
        bottomFace = Plane(corners[6], corners[7], corners[2]);  // bottom = nbl, nbr, fbl


        /*
        glm::mat4 inverseViewMatrix = glm::inverse(viewMatrix);
        glm::vec3 viewPos = glm::vec3(inverseViewMatrix[3][0], inverseViewMatrix[3][1], inverseViewMatrix[3][2]);
        glm::vec3 camForward = -glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);
        glm::vec3 camRight = glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
        glm::vec3 camUp = glm::vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);
        float fov = 2.0f * atan(1.0f / projectionMatrix[1][1]);
        float aspectRatio = projectionMatrix[1][1] / projectionMatrix[0][0];
        float nearPlane = projectionMatrix[3][2] / (projectionMatrix[2][2] - 1.0f);
        float farPlane = projectionMatrix[3][2] / (projectionMatrix[2][2] + 1.0f);
        const float halfVSide = 2.0f * tan(fov * 0.5f) * farPlane;
        const float halfHSide = halfVSide * aspectRatio;
        const glm::vec3 frontMultFar = farPlane * camForward;
        nearFace = { viewPos + nearPlane * camForward, camForward };
        farFace = { viewPos + frontMultFar, -camForward };
        rightFace = { viewPos, glm::cross(frontMultFar - camRight * halfHSide, camUp) };
        leftFace = { viewPos, glm::cross(camUp, frontMultFar + camRight * halfHSide) };
        topFace = { viewPos, glm::cross(camRight, frontMultFar - camUp * halfVSide) };
        bottomFace = { viewPos, glm::cross(frontMultFar + camUp * halfVSide, camRight) };*/
    }

    bool AABBIsOnOrForwardPlane(const Plane& plane, const glm::vec3 center, const glm::vec3 extents) const {
        const float r = extents.x * std::abs(plane.normal.x) + extents.y * std::abs(plane.normal.y) + extents.z * std::abs(plane.normal.z);
        return -r <= plane.GetSignedDistanceToPlane(center);
    }

    bool ContainsAABB(const glm::mat4& worldMatrix, const glm::vec3 center, const glm::vec3 extents) {

        const glm::vec3 globalCenter = worldMatrix * glm::vec4(center, 1.f);
        const glm::vec3 right = glm::vec3(worldMatrix[0]) * extents.x;
        const glm::vec3 up = glm::vec3(worldMatrix[1]) * extents.y;
        const glm::vec3 forward = glm::vec3(-worldMatrix[2]) * extents.z;
        const float newIi = std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, right)) + std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, up)) + std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, forward));
        const float newIj = std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, right)) + std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, up)) + std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, forward));
        const float newIk = std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, right)) + std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, up)) + std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, forward));
        return (AABBIsOnOrForwardPlane(leftFace, center, extents) &&
            AABBIsOnOrForwardPlane(rightFace, center, extents) &&
            AABBIsOnOrForwardPlane(topFace, center, extents) &&
            AABBIsOnOrForwardPlane(bottomFace, center, extents) &&
            AABBIsOnOrForwardPlane(nearFace, center, extents) &&
            AABBIsOnOrForwardPlane(farFace, center, extents));
    }

    bool IsAABBInsideFrustum(const glm::vec3& aabbMin, const glm::vec3& aabbMax) {
        const Plane* planes[] = {
            &topFace,
            &bottomFace,
            &rightFace,
            &leftFace,
            &farFace,
            &nearFace
        };
        for (const Plane* plane : planes) {
            glm::vec3 positiveVertex = aabbMin;
            if (plane->normal.x >= 0) {
                positiveVertex.x = aabbMax.x;
            }
            if (plane->normal.y >= 0) {
                positiveVertex.y = aabbMax.y;
            }
            if (plane->normal.z >= 0) {
                positiveVertex.z = aabbMax.z;
            }
            if (plane->GetSignedDistanceToPlane(positiveVertex) < 0) {
                return false;
            }
        }
        return true;
    }

    std::vector<glm::vec3> GetFrustumCorners(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix) {

        glm::mat4 inverseViewMatrix = glm::inverse(viewMatrix);
        glm::vec3 viewPos = glm::vec3(inverseViewMatrix[3][0], inverseViewMatrix[3][1], inverseViewMatrix[3][2]);
        glm::vec3 camForward = -glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);
        glm::vec3 camRight = glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
        glm::vec3 camUp = glm::vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);
        float fov = 2.0f * atan(1.0f / projectionMatrix[1][1]);
        float aspectRatio = projectionMatrix[1][1] / projectionMatrix[0][0];
        float nearPlane = projectionMatrix[3][2] / (projectionMatrix[2][2] - 1.0f);
        float farPlane = projectionMatrix[3][2] / (projectionMatrix[2][2] + 1.0f);
        glm::vec3 fc = viewPos + camForward * farPlane;
        glm::vec3 nc = viewPos + camForward * nearPlane;
        float Hfar = 2.0f * tan(fov / 2) * farPlane;
        float Wfar = Hfar * aspectRatio;
        float Hnear = 2.0f * tan(fov / 2) * nearPlane;
        float Wnear = Hnear * aspectRatio;
        glm::vec3 up = camUp;
        glm::vec3 right = camRight;
        glm::vec3 ftl = fc + (up * Hfar / 2.0f) - (right * Wfar / 2.0f);
        glm::vec3 ftr = fc + (up * Hfar / 2.0f) + (right * Wfar / 2.0f);
        glm::vec3 fbl = fc - (up * Hfar / 2.0f) - (right * Wfar / 2.0f);
        glm::vec3 fbr = fc - (up * Hfar / 2.0f) + (right * Wfar / 2.0f);
        glm::vec3 ntl = nc + (up * Hnear / 2.0f) - (right * Wnear / 2.0f);
        glm::vec3 ntr = nc + (up * Hnear / 2.0f) + (right * Wnear / 2.0f);
        glm::vec3 nbl = nc - (up * Hnear / 2.0f) - (right * Wnear / 2.0f);
        glm::vec3 nbr = nc - (up * Hnear / 2.0f) + (right * Wnear / 2.0f);

        nearFace = Plane(ntr, ntl, nbl);
        farFace = Plane(ftr, ftl, fbl);
        bottomFace = Plane(nbl, nbr, fbr);
        topFace = Plane(ntl, ntr, ftr);
        rightFace = Plane(ntr, nbr, fbr);
        leftFace = Plane(ntl, nbl, fbl);

        return { ftl, ftr, fbl, fbr, ntl, ntr, nbl, nbr };
    }

    std::vector<Vertex> GetFrustumCornerLineVertices(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix, glm::vec3 color) {

        std::vector<glm::vec3> corners = GetFrustumCorners(projectionMatrix, viewMatrix);
        std::vector<Vertex> vertices;
        vertices.push_back(Vertex(corners[6], color));
        vertices.push_back(Vertex(corners[7], color));
        vertices.push_back(Vertex(corners[4], color));
        vertices.push_back(Vertex(corners[5], color));
        vertices.push_back(Vertex(corners[6], color));
        vertices.push_back(Vertex(corners[4], color));
        vertices.push_back(Vertex(corners[7], color));
        vertices.push_back(Vertex(corners[5], color));
        vertices.push_back(Vertex(corners[2], color));
        vertices.push_back(Vertex(corners[3], color));
        vertices.push_back(Vertex(corners[0], color));
        vertices.push_back(Vertex(corners[1], color));
        vertices.push_back(Vertex(corners[2], color));
        vertices.push_back(Vertex(corners[0], color));
        vertices.push_back(Vertex(corners[3], color));
        vertices.push_back(Vertex(corners[1], color));
        vertices.push_back(Vertex(corners[6], color));
        vertices.push_back(Vertex(corners[2], color));
        vertices.push_back(Vertex(corners[7], color));
        vertices.push_back(Vertex(corners[3], color));
        vertices.push_back(Vertex(corners[4], color));
        vertices.push_back(Vertex(corners[0], color));
        vertices.push_back(Vertex(corners[5], color));
        vertices.push_back(Vertex(corners[1], color));

        return vertices;
    }
};