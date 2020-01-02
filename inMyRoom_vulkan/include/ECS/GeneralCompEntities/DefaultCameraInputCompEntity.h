#pragma once

#include "ECS/ECStypes.h"

#include <glm/vec3.hpp>

#include <chrono>



#ifndef GAME_DLL
class DefaultCameraInputComp;
#endif

class DefaultCameraInputCompEntity
{
#ifndef GAME_DLL
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
    static std::vector<std::pair<std::string, MapType>> GetComponentInitMapFields();

    void Init();
    void Update(class CameraComp* const cameraComp_ptr, const std::chrono::duration<float> durationOfLastState);
    void AsyncInput(InputType input_type, void* struct_data, const std::chrono::duration<float> durationOfLastState);

    void Freeze();
    void Unfreeze();

private:
    void MoveCamera(float xRotation_rads, float yRotation_rads);
    void CalculateSnap(const std::chrono::duration<float> durationOfLastState);

private: // static_variable
    friend class DefaultCameraInputComp;
    static DefaultCameraInputComp* defaultCameraInputComp_ptr;

#endif
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
};