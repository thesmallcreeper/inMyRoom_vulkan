#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <mutex>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>



class CameraBaseClass
{
public:
    CameraBaseClass(glm::vec3 in_lookingDirection, glm::vec3 in_position, glm::vec3 in_up);
    virtual ~CameraBaseClass();

    void Freeze();
    void Unfreeze();

    void RefreshPublicVectors();

	void ToggleCullingDubugging();

    void MoveCamera(float xRotation_rads, float yRotation_rads);

    void MoveForward();
    void MoveBackward();
    void MoveRight();
    void MoveLeft();
    void MoveUp();
    void MoveDown();

    void StopMovingForward();
    void StopMovingBackward();
    void StopMovingRight();
    void StopMovingLeft();
    void StopMovingUp();
    void StopMovingDown();

    glm::vec3 cameraPosition;
    glm::vec3 cameraLookingDirection;

	glm::vec3 cullingPosition;
	glm::vec3 cullingLookingDirection;

    glm::vec3 upVector;

private:

    virtual std::pair<glm::vec3, glm::vec3> CalculateSnap(const std::chrono::duration<float> durationOfCurrentState) = 0;

protected:

    struct
    {
        bool movingForward;
        bool movingBackward;
        bool movingRight;
        bool movingLeft;
        bool movingUp;
        bool movingDown;
    } movementState;

    glm::vec3 position;
    glm::vec3 lookingDirection;

private:

    std::chrono::steady_clock::time_point lastSnapTimePoint;

	bool cullingDebugging = false;

    volatile bool freezed = true;
    std::mutex controlMutex;

};