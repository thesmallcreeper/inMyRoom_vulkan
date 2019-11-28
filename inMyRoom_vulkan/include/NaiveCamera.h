#pragma once

#include "CameraBaseClass.h"

class NaiveCamera:
    public CameraBaseClass
{
public:
    NaiveCamera(float in_cameraSpeed, glm::vec3 in_lookingDirection = glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3 in_position = glm::vec3(0.0, 0.0, 0.0), glm::vec3 in_up = glm::vec3(0.0, -1.0, 0.0));
    ~NaiveCamera() override;

private:
    std::pair<glm::vec3, glm::vec3> CalculateSnap(const std::chrono::duration<float> durationOfCurrentState) override;

    float cameraSpeed;
};