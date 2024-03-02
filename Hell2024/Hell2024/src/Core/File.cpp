#include "File.h"
#include "../Common.h"
#include "rapidjson/document.h"
#include <rapidjson/filereadstream.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <filesystem>
#include <sstream> 
#include <fstream>
#include <string>
#include <iostream>
#include "../Core/Scene.h"
#include "../Core/AssetManager.h"

namespace File {
	void SaveVec2(rapidjson::Value* object, std::string elementName, glm::vec2 vector, rapidjson::Document::AllocatorType& allocator);
	void SaveVec3(rapidjson::Value* object, std::string elementName, glm::vec3 vector, rapidjson::Document::AllocatorType& allocator);
	void SaveString(rapidjson::Value* object, std::string elementName, std::string string, rapidjson::Document::AllocatorType& allocator);
	void SaveBool(rapidjson::Value* object, std::string elementName, bool boolean, rapidjson::Document::AllocatorType& allocator);
	void SaveFloat(rapidjson::Value* object, std::string elementName, float number, rapidjson::Document::AllocatorType& allocator);
	void SaveInt(rapidjson::Value* object, std::string elementName, int number, rapidjson::Document::AllocatorType& allocator);
	void SaveIntArray(rapidjson::Value* object, std::string elementName, std::vector<int> integers, rapidjson::Document::AllocatorType& allocator);
	glm::vec2 ReadVec2(const rapidjson::Value& value, std::string name);
	glm::vec3 ReadVec3(const rapidjson::Value& value, std::string name);
	std::vector<int> ReadIntArray(const rapidjson::Value& value, std::string name);
	std::string ReadString(const rapidjson::Value& value, std::string name);
	//char* ReadText(const rapidjson::Value& value, std::string name);
	bool ReadBool(const rapidjson::Value& value, std::string name);
	float ReadFloat(const rapidjson::Value& value, std::string name);
	int ReadInt(const rapidjson::Value& value, std::string name);
}

void File::LoadMap(std::string mapName) {

	FILE* filepoint;
	errno_t err;
	std::string fileName = "res/maps/" + mapName;
	rapidjson::Document document;

	if ((err = fopen_s(&filepoint, fileName.c_str(), "rb")) != 0) {
		//fprintf(stderr, "cannot open file '%s': %s\n", fileName, strerror(err));
		std::cout << "Failed to open " << fileName << "\n";
		return;
	}
	else {
		char buffer[65536];
		rapidjson::FileReadStream is(filepoint, buffer, sizeof(buffer));
		document.ParseStream<0, rapidjson::UTF8<>, rapidjson::FileReadStream>(is);
		if (document.HasParseError()) {
			std::cout << "Error  : " << document.GetParseError() << '\n' << "Offset : " << document.GetErrorOffset() << '\n';
		}
		fclose(filepoint);
	}

    if (document.HasMember("Lights")) {
        const rapidjson::Value& objects = document["Lights"];
        for (rapidjson::SizeType i = 0; i < objects.Size(); i++) {
            glm::vec3 position = ReadVec3(objects[i], "position");
            glm::vec3 color = ReadVec3(objects[i], "color");
            float radius = ReadFloat(objects[i], "radius");
            float strength = ReadFloat(objects[i], "strength");
            int type = ReadInt(objects[i], "type");
            Light light(position, color, radius, strength, type);
            Scene::AddLight(light);
        }
    }


    if (document.HasMember("SpawnPoints")) {
        const rapidjson::Value& objects = document["SpawnPoints"];
        for (rapidjson::SizeType i = 0; i < objects.Size(); i++) {
            glm::vec3 position = ReadVec3(objects[i], "position");
            glm::vec3 rotation = ReadVec3(objects[i], "rotation");
            SpawnPoint spawnPoint {position, rotation};
            Scene::_spawnPoints.push_back(spawnPoint);
        }
    }

	if (document.HasMember("Walls")) {
		const rapidjson::Value& objects = document["Walls"];
		for (rapidjson::SizeType i = 0; i < objects.Size(); i++) {
			glm::vec3 begin = ReadVec3(objects[i], "begin");
			glm::vec3 end = ReadVec3(objects[i], "end");
			float height = ReadFloat(objects[i], "height");
			std::string materialName = ReadString(objects[i], "materialName");
			Wall wall(begin, end, height, AssetManager::GetMaterialIndex(materialName));
			Scene::AddWall(wall);
		}
	}

	if (false) {
		if (document.HasMember("GameObjects")) {
			const rapidjson::Value& objects = document["GameObjects"];
			for (rapidjson::SizeType i = 0; i < objects.Size(); i++) {

				glm::vec3 position = ReadVec3(objects[i], "position");
				glm::vec3 rotation = ReadVec3(objects[i], "rotation");
				glm::vec3 scale = ReadVec3(objects[i], "scale");
				std::string name = ReadString(objects[i], "name");
				std::string modelName = ReadString(objects[i], "modelName");
				std::string parentName = ReadString(objects[i], "parentName");

				std::vector<int> meshIndices = ReadIntArray(objects[i], "meshMaterialIndices");


				std::cout << "Loading game object: " << name << "\n";

				GameObject& gameObject = Scene::_gameObjects.emplace_back();
				gameObject._transform.position = position;
				gameObject._transform.rotation = rotation;
				gameObject._transform.scale = scale;
				gameObject.SetName(name);
				gameObject.SetParentName(parentName);
				gameObject.SetModel(modelName);
				gameObject._meshMaterialIndices = meshIndices;
			}
		}
	}



	if (document.HasMember("Windows")) {
		const rapidjson::Value& objects = document["Windows"];
		for (rapidjson::SizeType i = 0; i < objects.Size(); i++) {
			glm::vec3 position = ReadVec3(objects[i], "position");
			float rotation = ReadFloat(objects[i], "rotation");
			Window& window = Scene::_windows.emplace_back();
			window.position = position;
			window.rotation.y = rotation;
		}
	}

	if (document.HasMember("Doors")) {
		const rapidjson::Value& objects = document["Doors"];
		for (rapidjson::SizeType i = 0; i < objects.Size(); i++) {
			glm::vec3 position = ReadVec3(objects[i], "position");
			float rotation = ReadFloat(objects[i], "rotation");
			Door door(position, rotation);
			Scene::AddDoor(door);
		}
	}

	if (document.HasMember("Floors")) {
		const rapidjson::Value& objects = document["Floors"];
		for (rapidjson::SizeType i = 0; i < objects.Size(); i++) {
			glm::vec3 v1 = ReadVec3(objects[i], "v1");
			glm::vec3 v2 = ReadVec3(objects[i], "v2");
			glm::vec3 v3 = ReadVec3(objects[i], "v3");
			glm::vec3 v4 = ReadVec3(objects[i], "v4");

			float x1 = ReadFloat(objects[i], "x1");
			float x2 = ReadFloat(objects[i], "x2");
			float z1 = ReadFloat(objects[i], "z1");
			float z2 = ReadFloat(objects[i], "z2");
			float height = ReadFloat(objects[i], "height");

			float textureScale = ReadFloat(objects[i], "textureScale");
			int materialIndex = AssetManager::GetMaterialIndex(ReadString(objects[i], "materialName"));
			//Floor floor(x1, z1, x2, z2, height, materialIndex, textureScale);
			Floor floor(v1, v2, v3, v4, materialIndex, textureScale);
			Scene::AddFloor(floor);
		}
	}
}

void File::SaveMap(std::string mapName) {

	rapidjson::Document document;
	rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
	rapidjson::Value walls(rapidjson::kArrayType);
	rapidjson::Value doors(rapidjson::kArrayType);
	rapidjson::Value windows(rapidjson::kArrayType);
    rapidjson::Value floors(rapidjson::kArrayType);
    rapidjson::Value gameObjects(rapidjson::kArrayType);
    rapidjson::Value lights(rapidjson::kArrayType);
    rapidjson::Value spawnPoints(rapidjson::kArrayType);
	document.SetObject();

    for (SpawnPoint& spawnPoint: Scene::_spawnPoints) {
        rapidjson::Value object(rapidjson::kObjectType);
        SaveVec3(&object, "position", spawnPoint.position, allocator);
        SaveVec3(&object, "rotation", spawnPoint.rotation, allocator);
        spawnPoints.PushBack(object, allocator);
    }

    for (Light & light : Scene::_lights) {
        rapidjson::Value object(rapidjson::kObjectType);
        SaveVec3(&object, "position", light.position, allocator);
        SaveVec3(&object, "color", light.color, allocator);
        SaveFloat(&object, "radius", light.radius, allocator);
        SaveFloat(&object, "strength", light.strength, allocator);
        SaveInt(&object, "type", light.type, allocator);
        lights.PushBack(object, allocator);
    }

	for (GameObject& gameObject : Scene::_gameObjects) {
		rapidjson::Value object(rapidjson::kObjectType);


		//std::vector<int> _meshMaterialIndices;
		//OpenState _openState = OpenState::NONE;
		//OpenAxis _openAxis = OpenAxis::NONE;
		//float _maxOpenAmount = 0;
		//float _minOpenAmount = 0;
		//float _openSpeed = 0;

		SaveVec3(&object, "position", gameObject._transform.position, allocator);
		SaveVec3(&object, "rotation", gameObject._transform.rotation, allocator);
		SaveVec3(&object, "scale", gameObject._transform.scale, allocator);
		SaveString(&object, "name", gameObject._name, allocator);
		SaveString(&object, "parentName", gameObject._parentName, allocator);
		SaveIntArray(&object, "meshMaterialIndices", gameObject._meshMaterialIndices, allocator);
		SaveString(&object, "modelName", gameObject._model->_name, allocator);

		

		gameObjects.PushBack(object, allocator);
	}

	for (Wall& wall : Scene::_walls) {
		rapidjson::Value object(rapidjson::kObjectType);
		SaveVec3(&object, "begin", wall.begin, allocator);
		SaveVec3(&object, "end", wall.end, allocator);
		SaveFloat(&object, "height", wall.height, allocator);
		SaveString(&object, "materialName", AssetManager::GetMaterialNameByIndex(wall.materialIndex), allocator);
		walls.PushBack(object, allocator);
	}

	for (Window& window: Scene::_windows) {
		rapidjson::Value object(rapidjson::kObjectType);
		SaveVec3(&object, "position", window.position, allocator);
		SaveFloat(&object, "rotation", window.rotation.y, allocator);
		windows.PushBack(object, allocator);
	}

	for (Door& door : Scene::_doors) {
		rapidjson::Value object(rapidjson::kObjectType);
		SaveVec3(&object, "position", door.position, allocator);
		SaveFloat(&object, "rotation", door.rotation, allocator);
		doors.PushBack(object, allocator);
	}

	for (Floor& floor : Scene::_floors) {
		rapidjson::Value object(rapidjson::kObjectType);
		SaveVec3(&object, "v1", floor.v1.position, allocator);
		SaveVec3(&object, "v2", floor.v2.position, allocator);
		SaveVec3(&object, "v3", floor.v3.position, allocator);
		SaveVec3(&object, "v4", floor.v4.position, allocator);
		SaveFloat(&object, "textureScale", floor.textureScale, allocator);

		SaveFloat(&object, "x1", floor.x1, allocator);
		SaveFloat(&object, "x2", floor.x2, allocator);
		SaveFloat(&object, "z1", floor.z1, allocator);
		SaveFloat(&object, "z2", floor.z2, allocator);
		SaveFloat(&object, "height", floor.height, allocator);

		SaveString(&object, "materialName", AssetManager::GetMaterialNameByIndex(floor.materialIndex), allocator);
		floors.PushBack(object, allocator);
	}

	document.AddMember("Walls", walls, allocator);
	document.AddMember("Floors", floors, allocator);
	document.AddMember("Doors", doors, allocator);
    document.AddMember("Windows", windows, allocator);
    document.AddMember("GameObjects", gameObjects, allocator);
    document.AddMember("Lights", lights, allocator);
    document.AddMember("SpawnPoints", spawnPoints, allocator);

	rapidjson::StringBuffer strbuf;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(strbuf);
	document.Accept(writer);

	std::string data = strbuf.GetString();
	std::ofstream out("res/maps/" + mapName);
	out << data;
	out.close();

	std::cout << "Saved res/maps/" << mapName << "\n";

	Scene::RecreateDataStructures();
}

void File::SaveVec3(rapidjson::Value* object, std::string elementName, glm::vec3 vector, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value array(rapidjson::kArrayType);
	rapidjson::Value name(elementName.c_str(), allocator);
	array.PushBack(rapidjson::Value().SetFloat(vector.x), allocator);
	array.PushBack(rapidjson::Value().SetFloat(vector.y), allocator);
	array.PushBack(rapidjson::Value().SetFloat(vector.z), allocator);
	object->AddMember(name, array, allocator);
}

void File::SaveVec2(rapidjson::Value* object, std::string elementName, glm::vec2 vector, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value array(rapidjson::kArrayType);
	rapidjson::Value name(elementName.c_str(), allocator);
	array.PushBack(rapidjson::Value().SetFloat(vector.x), allocator);
	array.PushBack(rapidjson::Value().SetFloat(vector.y), allocator);
	object->AddMember(name, array, allocator);
}

void File::SaveString(rapidjson::Value* object, std::string elementName, std::string string, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value name(elementName.c_str(), allocator);
	rapidjson::Value value(rapidjson::kObjectType);
	value.SetString(string.c_str(), static_cast<rapidjson::SizeType>(string.length()), allocator);
	object->AddMember(name, value, allocator);
}

void File::SaveBool(rapidjson::Value* object, std::string elementName, bool boolean, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value name(elementName.c_str(), allocator);
	rapidjson::Value value(rapidjson::kObjectType);
	value.SetBool(boolean);
	object->AddMember(name, value, allocator);
}

void File::SaveFloat(rapidjson::Value* object, std::string elementName, float number, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value name(elementName.c_str(), allocator);
	rapidjson::Value value(rapidjson::kObjectType);
	value.SetFloat(number);
	object->AddMember(name, value, allocator);
}

void File::SaveInt(rapidjson::Value* object, std::string elementName, int number, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value name(elementName.c_str(), allocator);
	rapidjson::Value value(rapidjson::kObjectType);
	value.SetInt(number);
	object->AddMember(name, value, allocator);
}

void File::SaveIntArray(rapidjson::Value* object, std::string elementName, std::vector<int> integers, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value array(rapidjson::kArrayType);
	rapidjson::Value name(elementName.c_str(), allocator);	
	for (int i = 0; i < integers.size(); i++) {
		array.PushBack(rapidjson::Value().Set(integers[i]), allocator);
	}
	object->AddMember(name, array, allocator);
}


std::vector<int> File::ReadIntArray(const rapidjson::Value& value, std::string name) {
	std::vector<int> array;
	if (value.HasMember(name.c_str())) {
		const rapidjson::Value& element = value[name.c_str()];
		for (int i = 0; i < element.Size(); i++) {
			array.emplace_back(element[i].GetInt());
			std::cout << i << " \n";
		}
	}
	else
		std::cout << "'" << name << "' NOT FOUND IN MAP FILE\n";
	return array;
}


glm::vec3 File::ReadVec3(const rapidjson::Value& value, std::string name) {
	glm::vec3 v = glm::vec3(0, 0, 0);
	if (value.HasMember(name.c_str())) {
		const rapidjson::Value& element = value[name.c_str()];
		v.x = element[0].GetFloat();
		v.y = element[1].GetFloat();
		v.z = element[2].GetFloat();
	}
	else
		std::cout << "'" << name << "' NOT FOUND IN MAP FILE\n";
	return v;
}

glm::vec2 File::ReadVec2(const rapidjson::Value& value, std::string name) {
	glm::vec2 v = glm::vec2(0, 0);
	if (value.HasMember(name.c_str())) {
		const rapidjson::Value& element = value[name.c_str()];
		v.x = element[0].GetFloat();
		v.y = element[1].GetFloat();
	}
	else
		std::cout << "'" << name << "' NOT FOUND IN MAP FILE\n";
	return v;
}

std::string File::ReadString(const rapidjson::Value& value, std::string name) {
	std::string s = "NOT FOUND";
	if (value.HasMember(name.c_str()))
		s = value[name.c_str()].GetString();
	else
		std::cout << "'" << name << "' NOT FOUND IN MAP FILE\n";
	return s;
}

/*char* File::ReadText(const rapidjson::Value& value, std::string name) {
	std::string s = "NOT FOUND";
	if (value.HasMember(name.c_str())) {
		s = value[name.c_str()].GetString();
		return s;
	}
	else
		std::cout << "'" << name << "' NOT FOUND IN MAP FILE\n";
	char* cstr = new char[s.length() + 1];
	strcpy_s(cstr, s.length() + 1, s.c_str());

	strcpy(cstr, s.c_str());
	// do stuff
	//delete[] cstr;		
	return cstr;
}*/

bool File::ReadBool(const rapidjson::Value& value, std::string name) {
	bool b = false;
	if (value.HasMember(name.c_str()))
		b = value[name.c_str()].GetBool();
	else
		std::cout << "'" << name << "' NOT FOUND IN MAP FILE\n";
	return b;
}

float File::ReadFloat(const rapidjson::Value& value, std::string name) {
	float f = -1;

	if (value.HasMember(name.c_str()))
		f = value[name.c_str()].GetFloat();
	else
		std::cout << "'" << name << "' NOT FOUND IN MAP FILE\n";
	return f;
}

int File::ReadInt(const rapidjson::Value& value, std::string name) {
	int f = -1;

	if (value.HasMember(name.c_str()))
		f = value[name.c_str()].GetInt();
	else
		std::cout << "'" << name << "' NOT FOUND IN MAP FILE\n";
	return f;
}
