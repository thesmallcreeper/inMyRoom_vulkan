#include "NaiveCamera.h"

NaiveCamera::NaiveCamera(float in_cameraSpeed, glm::vec3 in_lookingDirection, glm::vec3 in_position, glm::vec3 in_up)
    :CameraBaseClass(in_lookingDirection, in_position, in_up),
     cameraSpeed(in_cameraSpeed)
{

}

NaiveCamera::~NaiveCamera()
{

}

std::pair<glm::vec3, glm::vec3> NaiveCamera::CalculateSnap(const std::chrono::duration<float> durationOfCurrentState)
{
    glm::vec3 position_delta_vector(0.0f, 0.0f, 0.0f);

    if (movementState.movingForward) position_delta_vector += lookingDirection;
    if (movementState.movingBackward) position_delta_vector -= lookingDirection;

    if (movementState.movingRight) position_delta_vector += glm::normalize(glm::cross(lookingDirection, upVector));
    if (movementState.movingLeft) position_delta_vector -= glm::normalize(glm::cross(lookingDirection, upVector));

    if (movementState.movingUp) position_delta_vector += upVector;
    if (movementState.movingDown) position_delta_vector -= upVector;

    if (position_delta_vector.x != 0.0 || position_delta_vector.y != 0.0 || position_delta_vector.z != 0.0)
        position_delta_vector = durationOfCurrentState.count() * cameraSpeed * glm::normalize(position_delta_vector);


    // Pair first: position ,second: looking direction
    return std::make_pair(position + position_delta_vector, lookingDirection);
}
