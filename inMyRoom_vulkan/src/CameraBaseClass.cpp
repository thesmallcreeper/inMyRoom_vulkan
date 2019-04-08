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
    std::lock_guard<std::mutex> lock(controlMutex);
}

void CameraBaseClass::Freeze()
{
    std::lock_guard<std::mutex> lock(controlMutex);

    freezed = true;

    auto previous_snap_timePoint = lastSnapTimePoint;
    auto next_snap_timePoint = std::chrono::steady_clock::now();

    std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
    std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = CalculateSnap(duration);

    position = new_position_lookingDirection.first;
    position = new_position_lookingDirection.second;

    lastSnapTimePoint = next_snap_timePoint;
}

void CameraBaseClass::Unfreeze()
{
    std::lock_guard<std::mutex> lock(controlMutex);

    freezed = false;
    movementState.movingForward = false;
    movementState.movingBackward = false;
    movementState.movingRight = false;
    movementState.movingLeft = false;
    movementState.movingUp = false;
    movementState.movingDown = false;

    lastSnapTimePoint = std::chrono::steady_clock::now();
}

void CameraBaseClass::RefreshPublicVectors()
{
    std::lock_guard<std::mutex> lock(controlMutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = lastSnapTimePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = CalculateSnap(duration);

        position = new_position_lookingDirection.first;
        lookingDirection = new_position_lookingDirection.second;

        publicPosition = position;
        publicLookingDirection = lookingDirection;

        lastSnapTimePoint = next_snap_timePoint;

    }
}

void CameraBaseClass::MoveCamera(float xRotation_rads, float yRotation_rads)
{
    std::lock_guard<std::mutex> lock(controlMutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = lastSnapTimePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = CalculateSnap(duration);

        position = new_position_lookingDirection.first;
        lookingDirection = new_position_lookingDirection.second;

        lastSnapTimePoint = next_snap_timePoint;

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

void CameraBaseClass::MoveForward()
{
    std::lock_guard<std::mutex> lock(controlMutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = lastSnapTimePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = CalculateSnap(duration);

        position = new_position_lookingDirection.first;
        lookingDirection = new_position_lookingDirection.second;

        lastSnapTimePoint = next_snap_timePoint;

        movementState.movingForward = true;
    }
}

void CameraBaseClass::MoveBackward()
{
    std::lock_guard<std::mutex> lock(controlMutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = lastSnapTimePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = CalculateSnap(duration);

        position = new_position_lookingDirection.first;
        lookingDirection = new_position_lookingDirection.second;

        lastSnapTimePoint = next_snap_timePoint;

        movementState.movingBackward = true;
    }
}

void CameraBaseClass::MoveRight()
{
    std::lock_guard<std::mutex> lock(controlMutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = lastSnapTimePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = CalculateSnap(duration);

        position = new_position_lookingDirection.first;
        lookingDirection = new_position_lookingDirection.second;

        lastSnapTimePoint = next_snap_timePoint;

        movementState.movingRight = true;
    }
}
void CameraBaseClass::MoveLeft()
{
    std::lock_guard<std::mutex> lock(controlMutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = lastSnapTimePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = CalculateSnap(duration);

        position = new_position_lookingDirection.first;
        lookingDirection = new_position_lookingDirection.second;

        lastSnapTimePoint = next_snap_timePoint;

        movementState.movingLeft = true;
    }
}

void CameraBaseClass::MoveUp()
{
    std::lock_guard<std::mutex> lock(controlMutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = lastSnapTimePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = CalculateSnap(duration);

        position = new_position_lookingDirection.first;
        lookingDirection = new_position_lookingDirection.second;

        lastSnapTimePoint = next_snap_timePoint;

        movementState.movingUp = true;
    }
}

void CameraBaseClass::MoveDown() 
{
    std::lock_guard<std::mutex> lock(controlMutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = lastSnapTimePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = CalculateSnap(duration);

        position = new_position_lookingDirection.first;
        lookingDirection = new_position_lookingDirection.second;

        lastSnapTimePoint = next_snap_timePoint;

        movementState.movingDown = true;
    }
}


void CameraBaseClass::StopMovingForward()
{
    std::lock_guard<std::mutex> lock(controlMutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = lastSnapTimePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = CalculateSnap(duration);

        position = new_position_lookingDirection.first;
        lookingDirection = new_position_lookingDirection.second;

        lastSnapTimePoint = next_snap_timePoint;

        movementState.movingForward = false;
    }
}

void CameraBaseClass::StopMovingBackward()
{
    std::lock_guard<std::mutex> lock(controlMutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = lastSnapTimePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = CalculateSnap(duration);

        position = new_position_lookingDirection.first;
        lookingDirection = new_position_lookingDirection.second;

        lastSnapTimePoint = next_snap_timePoint;

        movementState.movingBackward = false;
    }
}

void CameraBaseClass::StopMovingRight()
{
    std::lock_guard<std::mutex> lock(controlMutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = lastSnapTimePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = CalculateSnap(duration);

        position = new_position_lookingDirection.first;
        lookingDirection = new_position_lookingDirection.second;

        lastSnapTimePoint = next_snap_timePoint;

        movementState.movingRight = false;
    }
}

void CameraBaseClass::StopMovingLeft()
{
    std::lock_guard<std::mutex> lock(controlMutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = lastSnapTimePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = CalculateSnap(duration);

        position = new_position_lookingDirection.first;
        lookingDirection = new_position_lookingDirection.second;

        lastSnapTimePoint = next_snap_timePoint;

        movementState.movingLeft = false;
    }
}

void CameraBaseClass::StopMovingUp()
{
    std::lock_guard<std::mutex> lock(controlMutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = lastSnapTimePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = CalculateSnap(duration);

        position = new_position_lookingDirection.first;
        lookingDirection = new_position_lookingDirection.second;

        lastSnapTimePoint = next_snap_timePoint;

        movementState.movingUp = false;
    }
}

void CameraBaseClass::StopMovingDown()
{
    std::lock_guard<std::mutex> lock(controlMutex);

    if (!freezed)
    {
        auto previous_snap_timePoint = lastSnapTimePoint;
        auto next_snap_timePoint = std::chrono::steady_clock::now();

        std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;
        std::pair<glm::vec3, glm::vec3> new_position_lookingDirection = CalculateSnap(duration);

        position = new_position_lookingDirection.first;
        lookingDirection = new_position_lookingDirection.second;

        lastSnapTimePoint = next_snap_timePoint;

        movementState.movingDown = false;
    }
}