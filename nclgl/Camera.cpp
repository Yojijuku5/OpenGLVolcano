#include "Camera.h"
#include "Window.h"
#include <algorithm>

void Camera::UpdateCamera(float dt) {

	/*
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_F)) {
		freeRoam = true;
	}
	if (!freeRoam) {
		return;
	}
	*/

	pitch -= (Window::GetMouse()->GetRelativePosition().y);
	yaw -= (Window::GetMouse()->GetRelativePosition().x);

	pitch = std::min(pitch, 90.0f);
	pitch = std::max(pitch, -90.0f);

	if (yaw < 0) {
		yaw += 360.0f;
	}
	if (yaw > 360.0f) {
		yaw -= 360.0f;
	}

	Matrix4 rotation = Matrix4::Rotation(yaw, Vector3(0, 1, 0));

	Vector3 forward = rotation * Vector3(0, 0, -1);
	Vector3 right = rotation * Vector3(1, 0, 0);

	float speed = 30.0f * dt;//default 30.0f

	if (Window::GetKeyboard()->KeyDown(KEYBOARD_W)) {
		position += forward * speed;
	}
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_S)) {
		position -= forward * speed;
	}
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_A)) {
		position -= right * speed;
	}
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_D)) {
		position += right * speed;
	}
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_SPACE)) {
		position.y += speed;
	}
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_SHIFT)) {
		position.y -= speed;
	}
}

Matrix4 Camera::BuildViewMatrix() {
	return Matrix4::Rotation(-pitch, Vector3(1, 0, 0)) * Matrix4::Rotation(-yaw, Vector3(0, 1, 0)) * Matrix4::Translation(-position);
}

//unused
void Camera::MoveCamera(Camera* camera) {
	if (isMoving) {
		Vector3 direction = targetPos - camera->position;

		if (direction.Length() < 0.1f) {
			isMoving = false;
			camera->position = targetPos;
		}
		else {
			direction = direction.Normalised() * speed;
			camera->position += direction;
		}
	}
}