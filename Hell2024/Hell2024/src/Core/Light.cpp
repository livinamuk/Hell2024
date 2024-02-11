#include "Light.h"
#include "AssetManager.h"
#include "../Util.hpp"
#include "../Core/Scene.h"

Light::Light() {

}

void Light::CleanUp() {
	for (int i = 0; i < Scene::_lights.size(); i++)
	{
		Scene::_lights.erase(Scene::_lights.begin() + i);
	}
}

void Light::CreateLightSource() {

	Model* model = AssetManager::GetModel("Lamp");
	if (!model) {
		std::cout << "Failed to create Window physics object, cause could not find 'Glass.obj' model\n";
		return;
	}

}