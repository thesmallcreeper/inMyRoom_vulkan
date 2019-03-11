#include "NaiveCamera.h"

NaiveCamera::NaiveCamera(float in_cameraSpeed, glm::vec3 in_lookingDirection, glm::vec3 in_position, glm::vec3 in_up)
	:MovementBaseClass(in_lookingDirection, in_position, in_up),
	 cameraSpeed(in_cameraSpeed)
{

}

NaiveCamera::~NaiveCamera()
{
    MovementBaseClass::~MovementBaseClass();
}

std::pair<glm::vec3, glm::vec3> NaiveCamera::calculate_snap(const std::chrono::duration<float> durationOfCurrentState)
{
	glm::vec3 positionDeltaVector(0.0f, 0.0f, 0.0f);

	if (movementState.movingForward) positionDeltaVector += lookingDirection;
	if (movementState.movingBackward) positionDeltaVector -= lookingDirection;

	if (movementState.movingRight) positionDeltaVector += glm::normalize(glm::cross(lookingDirection, upVector));
	if (movementState.movingLeft) positionDeltaVector -= glm::normalize(glm::cross(lookingDirection, upVector));

	if (movementState.movingUp) positionDeltaVector += upVector;
	if (movementState.movingDown) positionDeltaVector -= upVector;

	if (positionDeltaVector.x != 0.0 || positionDeltaVector.y != 0.0 || positionDeltaVector.z != 0.0)
		positionDeltaVector = durationOfCurrentState.count() * cameraSpeed * glm::normalize(positionDeltaVector);


	// Pair first: position ,second: looking direction
	return std::make_pair(position + positionDeltaVector, lookingDirection);
}