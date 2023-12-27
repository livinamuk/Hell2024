#include "Window.h"

Window::Window() {

}

glm::mat4 Window::GetModelMatrix() {
	Transform transform;
	transform.position = position;
	transform.rotation = rotation;
	return transform.to_mat4();
}