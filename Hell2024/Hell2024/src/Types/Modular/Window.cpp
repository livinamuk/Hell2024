#include "Window.h"
#include "../../Core/AssetManager.h"
#include "../../Util.hpp"

Window::Window() {

}

glm::mat4 Window::GetModelMatrix() {
	Transform transform;
	transform.position = m_position;
	transform.rotation.y = m_rotationY;
	return transform.to_mat4();
}

void Window::CleanUp() {

	// If the raycastBody exists, then they all do, so remove the old ones
	if (raycastBody) {
		raycastBody->release();
		//raycastBodyTop->release();
		raycastShape->release();
		//raycastShapeTop->release();
	}
}

void Window::CreatePhysicsObjects() {


    PxTriangleMesh* triangleMesh = Physics::CreateTriangleMeshFromModelIndex(AssetManager::GetModelIndexByName("Glass"));


	CleanUp(); // removes old PhysX objects


	{

		PhysicsFilterData filterData2;
		filterData2.raycastGroup = RaycastGroup::RAYCAST_ENABLED;
		filterData2.collisionGroup = NO_COLLISION;
		filterData2.collidesWith = NO_COLLISION;
		PxShapeFlags shapeFlags(PxShapeFlag::eSCENE_QUERY_SHAPE); // Most importantly NOT eSIMULATION_SHAPE. PhysX does not allow for tri mesh.
		raycastShape = Physics::CreateShapeFromTriangleMesh(triangleMesh, shapeFlags);
		raycastBody = Physics::CreateRigidStatic(Transform(), filterData2, raycastShape);

		PhysicsObjectData* physicsObjectData = new PhysicsObjectData(ObjectType::GLASS, this);
		raycastBody->userData = physicsObjectData;

		PxMat44 m2 = Util::GlmMat4ToPxMat44(GetModelMatrix());
		PxTransform transform2 = PxTransform(m2);
		raycastBody->setGlobalPose(transform2);
	}/*
	{
		OpenGLMesh* mesh = &model->_meshes[1];
		if (!mesh->_triangleMesh) {
			mesh->CreateTriangleMesh();
		}

		PhysicsFilterData filterData2;
		filterData2.raycastGroup = RaycastGroup::RAYCAST_ENABLED;
		filterData2.collisionGroup = NO_COLLISION;
		filterData2.collidesWith = NO_COLLISION;
		PxShapeFlags shapeFlags(PxShapeFlag::eSCENE_QUERY_SHAPE); // Most importantly NOT eSIMULATION_SHAPE. PhysX does not allow for tri mesh.
		raycastShapeTop = Physics::CreateShapeFromTriangleMesh(mesh->_triangleMesh, shapeFlags);
		raycastBodyTop = Physics::CreateRigidStatic(Transform(), filterData2, raycastShapeTop);

		PhysicsObjectData* physicsObjectData = new PhysicsObjectData(PhysicsObjectType::GLASS, this);
		raycastBodyTop->userData = physicsObjectData;

		PxMat44 m2 = Util::GlmMat4ToPxMat44(GetModelMatrix());
		PxTransform transform2 = PxTransform(m2);
		raycastBodyTop->setGlobalPose(transform2);
	}*/

}

glm::vec3 Window::GetFrontLeftCorner() {
	glm::vec4 result(0);
	result.z -= 0.1f;
	result.y += WINDOW_HEIGHT;
	result.x += WINDOW_WIDTH * 0.5f;
	result.a = 1.0f;
	glm::vec4 worldSpaceResult = GetModelMatrix() * result;
	return worldSpaceResult;
}

glm::vec3 Window::GetFrontRightCorner() {
	glm::vec4 result(0);
	result.z -= 0.1;
	result.y += WINDOW_HEIGHT;
	result.x -= WINDOW_WIDTH * 0.5f;
	result.a = 1;
	glm::vec4 worldSpaceResult = GetModelMatrix() * result;
	return worldSpaceResult;
}

glm::vec3 Window::GetBackLeftCorner() {
	glm::vec4 result(0);
	result.z += 0.1;
	result.y += WINDOW_HEIGHT;
	result.x -= WINDOW_WIDTH * 0.5f;
	result.a = 1;
	glm::vec4 worldSpaceResult = GetModelMatrix() * result;
	return worldSpaceResult;
}

glm::vec3 Window::GetBackRightCorner() {
	glm::vec4 result(0);
	result.z += 0.1;
	result.y += WINDOW_HEIGHT;
	result.x += WINDOW_WIDTH * 0.5f;
	result.a = 1;
	glm::vec4 worldSpaceResult = GetModelMatrix() * result;
	return worldSpaceResult;
}

glm::vec3 Window::GetWorldSpaceCenter() {
    return m_position + glm::vec3(0, 1.5f, 0);
}


void Window::UpdateRenderItems() {

    renderItems.clear();

    static uint32_t interiorMaterialIndex = AssetManager::GetMaterialIndex("Window");
    static uint32_t exteriorMaterialIndex = AssetManager::GetMaterialIndex("WindowExterior");

    Model* model = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Window"));

    for (int i = 0; i < model->GetMeshIndices().size(); i++) {
        uint32_t& meshIndex = model->GetMeshIndices()[i];
        uint32_t materialIndex = interiorMaterialIndex;
        if (i == 4 || i == 5 || i == 6) {
            materialIndex = exteriorMaterialIndex;
        }
        Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
        RenderItem3D& renderItem = renderItems.emplace_back();
        renderItem.vertexOffset = mesh->baseVertex;
        renderItem.indexOffset = mesh->baseIndex;
        renderItem.modelMatrix = GetModelMatrix();
        renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
        renderItem.meshIndex = meshIndex;
        Material* material = AssetManager::GetMaterialByIndex(materialIndex);
        renderItem.baseColorTextureIndex = material->_basecolor;
        renderItem.rmaTextureIndex = material->_rma;
        renderItem.normalMapTextureIndex = material->_normal;
    }
}

std::vector<RenderItem3D>& Window::GetRenderItems() {
    return renderItems;
}

glm::mat4 Window::GetGizmoMatrix() {
    Transform transform;
    transform.position = m_position + glm::vec3(0, 1.5f, 0);
    transform.rotation.y = m_rotationY;
    return transform.to_mat4();
}

void Window::SetPosition(glm::vec3 position) {
    m_position = position;
    if (raycastBody) {
        PxMat44 worldMatrix = Util::GlmMat4ToPxMat44(GetModelMatrix());
        PxTransform transform2 = PxTransform(worldMatrix);
        raycastBody->setGlobalPose(transform2);
    }
}

void Window::SetRotationY(float rotationY) {
    m_rotationY = rotationY;
    if (raycastBody) {
        PxMat44 worldMatrix = Util::GlmMat4ToPxMat44(GetModelMatrix());
        PxTransform transform = PxTransform(worldMatrix);
        raycastBody->setGlobalPose(transform);
    }
}

void Window::Rotate90() {
    SetRotationY(m_rotationY + HELL_PI * 0.5f);
}

glm::vec3 Window::GetPosition() {
    return m_position;
}

const float Window::GetPostionX() {
    return m_position.x;
}

const float Window::GetPostionY() {
    return m_position.y;
}

const float Window::GetPostionZ() {
    return m_position.z;
}

float Window::GetRotationY() {
    return m_rotationY;
}