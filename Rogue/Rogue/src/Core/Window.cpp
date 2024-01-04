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

}

void Window::CreatePhysicsObjects() {

	Model* model = AssetManager::GetModel("Glass");
	if (!model) {
		std::cout << "Failed to create Window physics object, cause could not find 'Glass.obj' model\n";
		return;
	}
	{
		Mesh* mesh = &model->_meshes[0];
		if (!mesh->_triangleMesh) {
			mesh->CreateTriangleMesh();
		}

		PhysicsFilterData filterData2;
		filterData2.raycastGroup = RaycastGroup::RAYCAST_ENABLED;
		filterData2.collisionGroup = NO_COLLISION;
		filterData2.collidesWith = NO_COLLISION;
		raycastShape = Physics::CreateShapeFromTriangleMesh(mesh->_triangleMesh);
		raycastBody = Physics::CreateRigidDynamic(Transform(), filterData2, raycastShape);
		raycastBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);

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
		raycastShape = Physics::CreateShapeFromTriangleMesh(mesh->_triangleMesh);
		raycastBody = Physics::CreateRigidDynamic(Transform(), filterData2, raycastShape);
		raycastBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);

		PhysicsObjectData* physicsObjectData = new PhysicsObjectData(PhysicsObjectType::GLASS, this);
		raycastBody->userData = physicsObjectData;

		PxMat44 m2 = Util::GlmMat4ToPxMat44(GetModelMatrix());
		PxTransform transform2 = PxTransform(m2);
		raycastBody->setGlobalPose(transform2);
	}

}