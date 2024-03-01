#include "Window.h"
#include "AssetManager.h"
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
		raycastBodyTop->release();
		raycastShape->release();
		raycastShapeTop->release();
	}
}

void Window::CreatePhysicsObjects() {

	Model* model = AssetManager::GetModel("Glass");
	if (!model) {
		std::cout << "Failed to create Window physics object, cause could not find 'Glass.obj' model\n";
		return;
	}

	CleanUp(); // removes old PhysX objects


	{

		Mesh* mesh = &model->_meshes[0];
		if (!mesh->_triangleMesh) {
			mesh->CreateTriangleMesh();
		}

		PhysicsFilterData filterData2;
		filterData2.raycastGroup = RaycastGroup::RAYCAST_ENABLED;
		filterData2.collisionGroup = NO_COLLISION;
		filterData2.collidesWith = NO_COLLISION;
		PxShapeFlags shapeFlags(PxShapeFlag::eSCENE_QUERY_SHAPE); // Most importantly NOT eSIMULATION_SHAPE. PhysX does not allow for tri mesh.
		raycastShape = Physics::CreateShapeFromTriangleMesh(mesh->_triangleMesh, shapeFlags);
		raycastBody = Physics::CreateRigidStatic(Transform(), filterData2, raycastShape);

		PhysicsObjectData* physicsObjectData = new PhysicsObjectData(PhysicsObjectType::GLASS, this);
		raycastBody->userData = physicsObjectData;

		PxMat44 m2 = Util::GlmMat4ToPxMat44(GetModelMatrix());
		PxTransform transform2 = PxTransform(m2);
		raycastBody->setGlobalPose(transform2);
	}
	{
		Mesh* mesh = &model->_meshes[1];
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
	}

}

glm::vec3 Window::GetFrontLeftCorner() {
	glm::vec4 result(0);
	result.z -= 0.1;
	result.y += WINDOW_HEIGHT;
	result.x += WINDOW_WIDTH * 0.5f;
	result.a = 1;
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