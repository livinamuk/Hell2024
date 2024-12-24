#include "ChristmasLights.h"
#include "../Core/AssetManager.h"
#include "../Game/Scene.h"

float ChristmasLerp(float start, float end, float t) {
    return start + t * (end - start);
}

std::vector<glm::vec3> GenerateSagPoints(const glm::vec3& start, const glm::vec3& end, int numPoints, float sagAmount) {
    std::vector<glm::vec3> points;
    float totalDistanceX = end.x - start.x;
    float totalDistanceZ = end.z - start.z;
    for (int i = 0; i < numPoints; ++i) {
        float t = static_cast<float>(i) / (numPoints - 1);
        float x = start.x + t * totalDistanceX;
        float z = start.z + t * totalDistanceZ;
        float y = ChristmasLerp(start.y, end.y, t);
        float sag = sagAmount * (4.0f * (t - 0.5f) * (t - 0.5f) - 1.0f);
        y += sag;
        points.push_back(glm::vec3(x, y, z));
    }
    return points;
}

std::vector<glm::vec3> GenerateCirclePoints(const glm::vec3& center, const glm::vec3& forward, float radius, int numPoints) {
    std::vector<glm::vec3> points;
    points.reserve(numPoints);

    // Normalize forward vector and compute orthogonal vectors
    glm::vec3 normalizedForward = glm::normalize(forward);
    glm::vec3 arbitrary = glm::abs(normalizedForward.x) < 0.9f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
    glm::vec3 right = glm::normalize(glm::cross(normalizedForward, arbitrary));
    glm::vec3 up = glm::normalize(glm::cross(right, normalizedForward));

    // Generate points around the circle
    for (int i = 0; i < numPoints; ++i) {
        float angle = (2.0f * glm::pi<float>() * i) / numPoints;
        glm::vec3 offset = radius * (std::cos(angle) * right + std::sin(angle) * up);
        points.push_back(center + offset);
    }

    return points;
}



// ShaderToy Simple Spiral
// https://www.shadertoy.com/view/MslyWB 
std::vector<glm::vec3> GenerateSpiralPoints(glm::vec3 spiralCenter, float radius, int numPoints, float numCycles, float spiralPower) {
    std::vector<glm::vec3> points;
    float spiralHeight = 5.0f;
    float spiralSag = 0.01f;
    for (int i = 0; i < numPoints; ++i) {
        float t = float(i) / float(numPoints);
        float r = std::pow(t * radius, spiralPower);
        float theta = numCycles * HELL_PI * t * 10;
        float x = r * std::cos(theta);
        float z = r * std::sin(theta);
        float y = float(i) / float(spiralHeight) * -spiralSag;
        points.emplace_back(spiralCenter + glm::vec3(x, y, z));
    }
    return points;
}


void ChristmasLights::Init(ChristmasLightsCreateInfo createInfo) {
    m_start = createInfo.start;
    m_end = createInfo.end;
    m_sag = 0.5f;// createInfo.sag;
    m_spiral = createInfo.spiral;

    float wireLength = glm::distance(m_start, m_end);
    wireLength = std::min(wireLength, 1.0f);

    int numPoints = wireLength * 25.0;
    int numOfLights = numPoints * 1.5f;
    float wireRadius = 0.001f;
    float wireCircleSegments = 5;


    if (!m_spiral) {
        m_wireSegmentPoints = GenerateSagPoints(m_start, m_end, numPoints, m_sag);
        m_lightSpawnPoints = GenerateSagPoints(m_start, m_end, numOfLights, m_sag);
        //std::cout << " Creating straight Christmas lights: " <<  m_wireSegmentPoints.size() << "\n";
    } 
    else {
        // Draw spiral
        glm::vec3 spiralCenter = glm::vec3(8.27, 4.56, 1.04);
        float radius = 0.6f;
        numPoints = 600; 
        numOfLights = numPoints * 1.5f;
        float numCycles = 4.5f;
        float spiralPower = 1.0f;
    
        std::vector truePoints = GenerateSpiralPoints(spiralCenter, radius, numOfLights, numCycles, spiralPower);
    
        for (int i = 0; i < truePoints.size() / 2; i++) {
            m_wireSegmentPoints.push_back(truePoints[i * 2]);
            m_lightSpawnPoints.push_back(truePoints[i * 2]);
        }
    }

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for (int j = 0; j < m_wireSegmentPoints.size() -1; j++) {
        glm::vec3& p0 = m_wireSegmentPoints[j];
        glm::vec3& p1 = m_wireSegmentPoints[j+1];
        glm::vec3 forward = p0 - p1;
        const std::vector<glm::vec3>& circle1 = GenerateCirclePoints(p0, forward, wireRadius, wireCircleSegments);
        const std::vector<glm::vec3>& circle2 = GenerateCirclePoints(p1, forward, wireRadius, wireCircleSegments);
        size_t pointCount = circle1.size();
        glm::vec3 center1(0.0f), center2(0.0f);
        for (const auto& p : circle1) center1 += p;
        for (const auto& p : circle2) center2 += p;
        center1 /= (float)pointCount;
        center2 /= (float)pointCount;

        for (size_t i = 0; i < pointCount; ++i) {
            size_t next = (i + 1) % pointCount;
            // Positions
            glm::vec3 pos1 = circle1[i];
            glm::vec3 pos2 = circle1[next];
            glm::vec3 pos3 = circle2[i];
            glm::vec3 pos4 = circle2[next];
            // Normals
            glm::vec3 normal1 = glm::normalize(pos1 - center1);
            glm::vec3 normal2 = glm::normalize(pos2 - center1);
            glm::vec3 normal3 = glm::normalize(pos3 - center2);
            glm::vec3 normal4 = glm::normalize(pos4 - center2);
            // UVs
            glm::vec2 uv1(i / (float)pointCount, 0.0f);
            glm::vec2 uv2((next) / (float)pointCount, 0.0f);
            glm::vec2 uv3(i / (float)pointCount, 1.0f);
            glm::vec2 uv4((next) / (float)pointCount, 1.0f);
            // Tangents
            glm::vec3 edge1 = pos2 - pos1;
            glm::vec3 edge2 = pos3 - pos1;
            glm::vec2 deltaUV1 = uv2 - uv1;
            glm::vec2 deltaUV2 = uv3 - uv1;
            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
            glm::vec3 tangent1;
            tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
            tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
            tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
            tangent1 = glm::normalize(tangent1);
            edge1 = pos4 - pos2;
            edge2 = pos3 - pos2;
            deltaUV1 = uv4 - uv2;
            deltaUV2 = uv3 - uv2;
            glm::vec3 tangent2;
            tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
            tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
            tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
            tangent2 = glm::normalize(tangent2);
            // Populate vectors
            uint32_t idx1 = (uint32_t)vertices.size();
            vertices.emplace_back(pos1, normal1, uv1, tangent1);
            vertices.emplace_back(pos2, normal2, uv2, tangent1);
            vertices.emplace_back(pos3, normal3, uv3, tangent2);
            vertices.emplace_back(pos4, normal4, uv4, tangent2);
            indices.push_back(idx1);
            indices.push_back(idx1 + 1);
            indices.push_back(idx1 + 2);
            indices.push_back(idx1 + 1);
            indices.push_back(idx1 + 3);
            indices.push_back(idx1 + 2);
        }
    }
    //std::cout << "vertices: " << vertices.size() << " indices: " << indices.size() << "\n";
    g_wireMesh.UpdateVertexBuffer(vertices, indices);

    static Model* model = AssetManager::GetModelByName("ChristmasLight2");
    static int whiteMaterialIndex = AssetManager::GetMaterialIndex("ChristmasLightWhite");
    static int blackMaterialIndex = AssetManager::GetMaterialIndex("Black");

    int j = 0;
    m_renderItems.clear();
    for (int i = 0; i < m_lightSpawnPoints.size(); i++) {

        Transform transform;
        transform.position = m_lightSpawnPoints[i];
        transform.rotation = glm::vec3(Util::RandomFloat(-1, 1), Util::RandomFloat(-1, 1), Util::RandomFloat(-1, 1));

        // Plastic
        RenderItem3D renderItem;
        renderItem.meshIndex = model->GetMeshIndices()[0];;
        renderItem.modelMatrix = transform.to_mat4();
        renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);
        Material* material = AssetManager::GetMaterialByIndex(blackMaterialIndex);
        renderItem.baseColorTextureIndex = material->_basecolor;
        renderItem.rmaTextureIndex = material->_rma;
        renderItem.normalMapTextureIndex = material->_normal;
        renderItem.castShadow = false;
        m_renderItems.push_back(renderItem);


        // Blue = 65.0 / 255.0, 152.065.0 / 255.0, 220.0 65.0 / 255.0
        // 2555 224 27 yellow
        // Blue = 65.0 / 255.0, 152.065.0 / 255.0, 220.0 65.0 / 255.0
        static glm::vec3 yellow = glm::vec3(247.0 / 255.0, 216.0 / 255.0, 12.0 / 255.0);
        static glm::vec3 blue = glm::vec3(22.0 / 255.0, 220.0 / 255.0, 230.0 / 255.0);
        static glm::vec3 green = glm::vec3(30.0 / 255.0, 121.0 / 255.0, 44.0 / 255.0);

        // 0.29, 0.49, 0.980.29, 0.49, 0.98

        // Light color
        glm::vec3 color = RED;
        if (j == 1) {
            color = blue;
        }
        else if (j == 2) {
            color = yellow;
        }
        else if (j == 3) {
            color = green;
        }

        // Light
        renderItem.meshIndex = model->GetMeshIndices()[1];;
        material = AssetManager::GetMaterialByIndex(whiteMaterialIndex);
        renderItem.baseColorTextureIndex = material->_basecolor;
        renderItem.rmaTextureIndex = material->_rma;
        renderItem.normalMapTextureIndex = material->_normal;
        renderItem.useEmissiveMask = 1.0f;
        renderItem.castShadow = false;
        renderItem.emissiveColor = color;// glm::vec3(Util::RandomFloat(0, 1), Util::RandomFloat(0, 1), Util::RandomFloat(0, 1));
        m_renderItems.push_back(renderItem);

    //   LightCreateInfo lightCreateInfo;
    //   lightCreateInfo.position = m_lightSpawnPoints[i];
    //   lightCreateInfo.color = color;
    //   lightCreateInfo.radius = 0.3f;
    //   lightCreateInfo.strength = 1.0f;
    //   lightCreateInfo.type = -1;
    //   lightCreateInfo.shadowCasting = false;
    //   Scene::CreateLight(lightCreateInfo);

        j++;
        if (j == 4) {
            j = 0;
        }
    }
}