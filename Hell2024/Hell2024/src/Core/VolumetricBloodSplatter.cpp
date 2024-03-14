#include "VolumetricBloodSplatter.h"
#include "AssetManager.h"

GLuint VolumetricBloodSplatter::s_vao;
GLuint VolumetricBloodSplatter::s_buffer_mode_matrices;
unsigned int VolumetricBloodSplatter::s_counter = 0;

VolumetricBloodSplatter::VolumetricBloodSplatter(glm::vec3 position, glm::vec3 rotation, glm::vec3 front) {
    m_transform.position = position;
    m_transform.rotation = rotation;
    m_transform.scale = glm::vec3(0.5f);
    m_front = front;

    if (s_counter == 0)
        m_type = 7;
    else if (s_counter == 1)
        m_type = 9;
    else if (s_counter == 2)
        m_type = 6;
    else if (s_counter == 3)
        m_type = 4;

    s_counter++;

    if (s_counter == 4)
        s_counter = 0;
}

void VolumetricBloodSplatter::Update(float deltaTime) {
    m_CurrentTime += deltaTime * 1.5f;
}

glm::mat4 VolumetricBloodSplatter::GetModelMatrix() {
    Transform bloodMeshTransform;
    bloodMeshTransform.position = m_transform.position;
    bloodMeshTransform.rotation = m_transform.rotation;

    Transform bloodMeshOffset;

    if (m_type == 0)
        bloodMeshOffset.position = glm::vec3(-0.08f, -0.23f, -0.155f);
    else if (m_type == 7)
        bloodMeshOffset.position = glm::vec3(-0.2300000042f, -0.5000000000f, -0.2249999940f);
    else if (m_type == 6)
        bloodMeshOffset.position = glm::vec3(-0.0839999989, -0.3799999952, -0.1500000060);
    else if (m_type == 8)
        bloodMeshOffset.position = glm::vec3(-0.1700000018f, -0.2290000021f, -0.1770000011f);
    else if (m_type == 9)
        bloodMeshOffset.position = glm::vec3(-0.0500000007, -0.2549999952, -0.1299999952);
    else if (m_type == 4)
        bloodMeshOffset.position = glm::vec3(-0.0500000045f, -0.4149999917f, -0.1900000125f);


    //	bloodMeshOffset.position = glm::vec3(Config::TEST_FLOAT, Config::TEST_FLOAT2, Config::TEST_FLOAT3);

    //bloodMeshOffset.position = glm::vec3(Config::TEST_FLOAT, Config::TEST_FLOAT2, Config::TEST_FLOAT3);
        //bloodMeshOffset.position = glm::vec3(-0.0500000007, -0.2549999952, -0.1299999952);
    //	bloodMeshOffset.position = glm::vec3(-0.231f, -0.492f, -0.225f);

    //bloodMeshOffset.position = glm::vec3(Config::TEST_FLOAT, Config::TEST_FLOAT2, Config::TEST_FLOAT3);

    Transform rotTransform; // Rotates mesh 90 de	gress around Y axis
    rotTransform.rotation = glm::vec3(0, -HELL_PI / 2, 0);
    //rotTransform.rotation = glm::vec3(Config::TEST_FLOAT, Config::TEST_FLOAT2, Config::TEST_FLOAT3);

    Transform scaleTransform;
    scaleTransform.scale = glm::vec3(3);

    return bloodMeshTransform.to_mat4() * scaleTransform.to_mat4() * rotTransform.to_mat4() * bloodMeshOffset.to_mat4();
}


void VolumetricBloodSplatter::Draw(Shader* shader)
{
    shader->Use();
    shader->SetMat4("u_MatrixWorld", GetModelMatrix());
    shader->SetFloat("u_Time", this->m_CurrentTime);

    Model* m_model = NULL;

    if (m_type == 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, AssetManager::s_ExrTexture_pos.gTexId);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, AssetManager::s_ExrTexture_norm.gTexId);
        m_model = AssetManager::GetModel("blood_mesh");
    }
    else if (m_type == 7) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, AssetManager::s_ExrTexture_pos7.gTexId);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, AssetManager::s_ExrTexture_norm7.gTexId);
        m_model = AssetManager::GetModel("blood_mesh7");
    }
    else if (m_type == 6) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, AssetManager::s_ExrTexture_pos6.gTexId);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, AssetManager::s_ExrTexture_norm6.gTexId);
        m_model = AssetManager::GetModel("blood_mesh6");
    }
    else if (m_type == 8) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, AssetManager::s_ExrTexture_pos8.gTexId);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, AssetManager::s_ExrTexture_norm8.gTexId);
        m_model = AssetManager::GetModel("blood_mesh8");
    }
    else if (m_type == 9) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, AssetManager::s_ExrTexture_pos9.gTexId);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, AssetManager::s_ExrTexture_norm9.gTexId);
        m_model = AssetManager::GetModel("blood_mesh9");
    }
    else if (m_type == 4) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, AssetManager::s_ExrTexture_pos4.gTexId);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, AssetManager::s_ExrTexture_norm4.gTexId);
        m_model = AssetManager::GetModel("blood_mesh4");
    }

    int VAO = m_model->_meshes[0]._VAO;
    GLsizei numIndices = (GLsizei)m_model->_meshes[0].indices.size();
    GLsizei numVerts = (GLsizei)m_model->_meshes[0].vertices.size();

    glBindVertexArray(VAO);
    //glPointSize(4);
    glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0);
}