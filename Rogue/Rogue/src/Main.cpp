
#include "Engine.h"
#include "Core/VoxelWorld.h"

#include "common.h"

int main() {

	glm::vec3 normalA = glm::vec3(0, 1, 0);
	glm::vec3 normalB = glm::normalize(glm::vec3(0, 1, 1));

	std::cout << glm::dot(normalA, normalB);

	glm::vec3 normalC = glm::normalize(glm::vec3(0, 1, 1));
	glm::vec3 normalD = glm::normalize(glm::vec3(0, 1, 2));
	glm::vec3 normalE = glm::normalize(glm::vec3(0, 1, 4));

	float dotAB = glm::dot(normalA, normalB);
	float dotAC = glm::dot(normalA, normalC);
	float dotAD = glm::dot(normalA, normalD);
	float dotAE = glm::dot(normalA, normalE);

	std::cout << dotAB << " " << glm::acos(dotAB) << " " << glm::degrees(glm::acos(dotAB)) << "\n";
	std::cout << dotAC << " " << glm::acos(dotAC) << " " << glm::degrees(glm::acos(dotAC)) << "\n";
	std::cout << dotAD << " " << glm::acos(dotAD) << " " << glm::degrees(glm::acos(dotAD)) << "\n";
	std::cout << dotAE << " " << glm::acos(dotAE) << " " << glm::degrees(glm::acos(dotAE)) << "\n";

	std::cout << "\n";

	std::cout << glm::radians(45.0f) << "\n";
	std::cout << glm::cos(glm::radians(45.0f)) << "\n";



//	return 0;
   Engine::Run();
   return 0;
}


