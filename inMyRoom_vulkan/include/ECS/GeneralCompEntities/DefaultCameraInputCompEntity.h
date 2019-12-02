#pragma once

#include "ECS/ECStypes.h"

#include <chrono>

#include <glm/vec3.hpp>


class DefaultCameraInputComp;

class DefaultCameraInputCompEntity
{
public:
    DefaultCameraInputCompEntity(const Entity this_entity);
    ~DefaultCameraInputCompEntity();

    static DefaultCameraInputCompEntity GetEmpty();

    /*  CreateComponentEntityByMap - DefaultCameraInputCompEntity
    "freezed",           isfreezed               = int
    "speed",             speed                   = float         (optional-default from config)
    "GlobalPosition",    globalPosition          = vec4.xyz      (optional-default  0, 0, 0)
    "GlobalDirection",   globalDirection         = vec4.xyz      (optional-default  0, 0, 1)
    "UpDirection",       upDirection             = vec4.xyz      (optional-default  0,-1, 0)
    */
    static DefaultCameraInputCompEntity CreateComponentEntityByMap(const Entity in_entity, const CompEntityInitMap in_map);

    void Init();

    void Update(class CameraComp* const cameraComp_ptr, const std::chrono::duration<float> durationOfLastState);
    void AsyncInput(InputType input_type, void* struct_data, const std::chrono::duration<float> durationOfLastState);

    void Freeze();
    void Unfreeze();

public: //data
    struct
    {
        bool movingForward = false;
        bool movingBackward = false;
        bool movingRight = false;
        bool movingLeft = false;
        bool movingUp = false;
        bool movingDown = false;
    } movementState;

    glm::vec3 globalPosition = glm::vec3(0.f, 0.f, 0.f);
    glm::vec3 globalDirection = glm::vec3(0.f, 0.f, 1.f);
    glm::vec3 upDirection = glm::vec3(0.f, -1.f, 0.f);

    float speed;
    volatile bool isFreezed;

    Entity thisEntity;

private:
    void MoveCamera(float xRotation_rads, float yRotation_rads);
    void CalculateSnap(const std::chrono::duration<float> durationOfLastState);

private: // static_variable
    friend class DefaultCameraInputComp;
    static DefaultCameraInputComp* defaultCameraInputComp_ptr;
};