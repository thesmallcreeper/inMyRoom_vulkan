#include "CameraBaseClass.h"

CameraBaseClass::CameraBaseClass(glm::vec3 in_lookingDirection, glm::vec3 in_position, glm::vec3 in_up)
    :lookingDirection(in_lookingDirection),
     position(in_position),
     upVector(in_up),
     freezed(true)
{

}

CameraBaseClass::~CameraBaseClass()
{
    std::lock_guard<std::mutex> lock(control_mutex);
}

void CameraBaseClass::freeze()
{
    std::lock_guard<std::mutex> lock(control_mutex);

    freezed = true;

    auto previous_snap_timePoint = last_snap_timePoint;
    auto next_snap_timePoint = std::chrono::steady_clock::now();

    std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
    std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = calculate_snap(duration);

    position = new_position_lookingDirection.first;
    position = new_position_lookingDirection.second;

    last_snap_timePoint = next_snap_timePoint;
}

void CameraBaseClass::unfreeze()
{
    std::lock_guard<std::mutex> lock(control_mutex);

    freezed = false;
    movementState.movingForward = false;
    movementState.movingBackward = false;
    movementState.movingRight = false;
    movementState.movingLeft = false;
    movementState.movingUp = false;
    movementState.movingDown = false;

    last_snap_timePoint = std::chrono::steady_clock::now();
}

glm::mat4x4 CameraBaseClass::getLookAtMatrix()
{
    std::lock_guard<std::mutex> lock(control_mutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = last_snap_timePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = calculate_snap(duration);

        return glm::lookAt(new_position_lookingDirection.first, new_position_lookingDirection.first + new_position_lookingDirection.second, upVector);
    }
    else
    {
        return glm::lookAt(position, position + lookingDirection, upVector);
    }

}

void CameraBaseClass::moveCamera(float xRotation_rads, float yRotation_rads)
{
    std::lock_guard<std::mutex> lock(control_mutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = last_snap_timePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = calculate_snap(duration);

        position = new_position_lookingDirection.first;
        lookingDirection = new_position_lookingDirection.second;

        last_snap_timePoint = next_snap_timePoint;

        glm::vec3 newLookingDirection = lookingDirection;

        newLookingDirection = glm::rotate(newLookingDirection, yRotation_rads, glm::normalize(glm::cross(newLookingDirection, upVector)));
        {
            const float minTheta = 0.01f * glm::half_pi<float>();
            const float maxAbsDirectionY = glm::cos(minTheta);
            const float minXZLength = glm::sin(minTheta);

            glm::vec2 oldXZorientation(lookingDirection.x, lookingDirection.z);
            glm::vec2 newXZorientation(newLookingDirection.x, newLookingDirection.z);

            if (( (glm::dot(oldXZorientation, newXZorientation) < 0.0f) || (glm::dot(oldXZorientation, newXZorientation) == 0.0f) || (glm::dot(oldXZorientation, newXZorientation) == -0.0f)
                || (std::abs(newLookingDirection.y) > maxAbsDirectionY) ) && (std::abs(newLookingDirection.y) > 0.5f))
            {
                glm::vec2 oldXZorientationNormalized = glm::normalize(oldXZorientation);
                newLookingDirection = glm::vec3(minXZLength * oldXZorientationNormalized.x, (lookingDirection.y > 0.0f) ? maxAbsDirectionY : -maxAbsDirectionY, minXZLength * oldXZorientationNormalized.y);
            }
        }
        newLookingDirection = glm::rotate(newLookingDirection, xRotation_rads, upVector);

        lookingDirection = glm::normalize(newLookingDirection);

    }
}

void CameraBaseClass::moveForward()
{
    std::lock_guard<std::mutex> lock(control_mutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = last_snap_timePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = calculate_snap(duration);

        position = new_position_lookingDirection.first;
        lookingDirection = new_position_lookingDirection.second;

        last_snap_timePoint = next_snap_timePoint;

        movementState.movingForward = true;
    }
}

void CameraBaseClass::moveBackward()
{
    std::lock_guard<std::mutex> lock(control_mutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = last_snap_timePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = calculate_snap(duration);

        position = new_position_lookingDirection.first;
        lookingDirection = new_position_lookingDirection.second;

        last_snap_timePoint = next_snap_timePoint;

        movementState.movingBackward = true;
    }
}

void CameraBaseClass::moveRight()
{
    std::lock_guard<std::mutex> lock(control_mutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = last_snap_timePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = calculate_snap(duration);

        position = new_position_lookingDirection.first;
        lookingDirection = new_position_lookingDirection.second;

        last_snap_timePoint = next_snap_timePoint;

        movementState.movingRight = true;
    }
}
void CameraBaseClass::moveLeft()
{
    std::lock_guard<std::mutex> lock(control_mutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = last_snap_timePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = calculate_snap(duration);

        position = new_position_lookingDirection.first;
        lookingDirection = new_position_lookingDirection.second;

        last_snap_timePoint = next_snap_timePoint;

        movementState.movingLeft = true;
    }
}

void CameraBaseClass::moveUp()
{
    std::lock_guard<std::mutex> lock(control_mutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = last_snap_timePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = calculate_snap(duration);

        position = new_position_lookingDirection.first;
        lookingDirection = new_position_lookingDirection.second;

        last_snap_timePoint = next_snap_timePoint;

        movementState.movingUp = true;
    }
}

void CameraBaseClass::moveDown() 
{
    std::lock_guard<std::mutex> lock(control_mutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = last_snap_timePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = calculate_snap(duration);

        position = new_position_lookingDirection.first;
        lookingDirection = new_position_lookingDirection.second;

        last_snap_timePoint = next_snap_timePoint;

        movementState.movingDown = true;
    }
}


void CameraBaseClass::stopMovingForward()
{
    std::lock_guard<std::mutex> lock(control_mutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = last_snap_timePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = calculate_snap(duration);

        position = new_position_lookingDirection.first;
        lookingDirection = new_position_lookingDirection.second;

        last_snap_timePoint = next_snap_timePoint;

        movementState.movingForward = false;
    }
}

void CameraBaseClass::stopMovingBackward()
{
    std::lock_guard<std::mutex> lock(control_mutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = last_snap_timePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = calculate_snap(duration);

        position = new_position_lookingDirection.first;
        lookingDirection = new_position_lookingDirection.second;

        last_snap_timePoint = next_snap_timePoint;

        movementState.movingBackward = false;
    }
}

void CameraBaseClass::stopMovingRight()
{
    std::lock_guard<std::mutex> lock(control_mutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = last_snap_timePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = calculate_snap(duration);

        position = new_position_lookingDirection.first;
        lookingDirection = new_position_lookingDirection.second;

        last_snap_timePoint = next_snap_timePoint;

        movementState.movingRight = false;
    }
}

void CameraBaseClass::stopMovingLeft()
{
    std::lock_guard<std::mutex> lock(control_mutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = last_snap_timePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = calculate_snap(duration);

        position = new_position_lookingDirection.first;
        lookingDirection = new_position_lookingDirection.second;

        last_snap_timePoint = next_snap_timePoint;

        movementState.movingLeft = false;
    }
}

void CameraBaseClass::stopMovingUp()
{
    std::lock_guard<std::mutex> lock(control_mutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = last_snap_timePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = calculate_snap(duration);

        position = new_position_lookingDirection.first;
        lookingDirection = new_position_lookingDirection.second;

        last_snap_timePoint = next_snap_timePoint;

        movementState.movingUp = false;
    }
}

void CameraBaseClass::stopMovingDown()
{
    std::lock_guard<std::mutex> lock(control_mutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = last_snap_timePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = calculate_snap(duration);

        position = new_position_lookingDirection.first;
        lookingDirection = new_position_lookingDirection.second;

        last_snap_timePoint = next_snap_timePoint;

        movementState.movingDown = false;
    }
}