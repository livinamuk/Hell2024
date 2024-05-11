#include "Window.h"
#include "../API/OpenGL/GL_assetManager.h"
#include "../Core/AssetManager.h"
#include "../Util.hpp"

Window::Window() {

}

glm::mat4 Window::GetModelMatrix() {
	Transform transform;
	transform.position = position;
	transform.rotation = rotation;
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

		PhysicsObjectData* physicsObjectData = new PhysicsObjectData(PhysicsObjectType::GLASS, this);
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
    return position + glm::vec3(0, 1.5f, 0);
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
        renderItem.baseColorTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_basecolor;
        renderItem.normalTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_normal;
        renderItem.rmaTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_rma;
    }
}

std::vector<RenderItem3D>& Window::GetRenderItems() {
    return renderItems;
}