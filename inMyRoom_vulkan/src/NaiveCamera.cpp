#include "NaiveCamera.h"

NaiveCamera::NaiveCamera(float in_cameraSpeed, glm::vec3 in_lookingDirection, glm::vec3 in_position, glm::vec3 in_up)
	:MovementBaseClass(in_lookingDirection, in_position, in_up),
	 cameraSpeed(in_cameraSpeed)
{

}

NaiveCamera::~NaiveCamera()
{

}

std::pair<glm::vec3, glm::vec3> NaiveCamera::calculate_snap(const std::chrono::duration<float> durationOfCurrentState)
{
	glm::vec3 positionOffset(0.0f, 0.0f, 0.0f);

	if (movementState.movingForward) positionOffset += lookingDirection;
	if (movementState.movingBackward) positionOffset -= lookingDirection;

	if (movementState.movingRight) positionOffset += glm::normalize(glm::cross(lookingDirection, upVector));
	if (movementState.movingLeft) positionOffset -= glm::normalize(glm::cross(lookingDirection, upVector));

	if (movementState.movingUp) positionOffset += upVector;
	if (movementState.movingDown) positionOffset -= upVector;

	if (positionOffset.x != 0.0 || positionOffset.y != 0.0 || positionOffset.z != 0.0)
		positionOffset = durationOfCurrentState.count() * cameraSpeed * glm::normalize(positionOffset);


	// Pair first: position ,second: looking direction
	return std::make_pair(position + positionOffset, lookingDirection);
}