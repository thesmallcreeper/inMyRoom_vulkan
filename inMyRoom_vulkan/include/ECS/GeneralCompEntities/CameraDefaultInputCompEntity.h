#pragma once

#include "ECS/CompEntityBaseWrappedClass.h"

#include <chrono>

class CameraDefaultInputComp;

#include "ECS/GeneralCompEntities/CameraCompEntity.h"

class CameraDefaultInputCompEntity :
    public CompEntityBaseWrappedClass<CameraDefaultInputComp>
{
#ifndef GAME_DLL
public:
    CameraDefaultInputCompEntity(Entity this_entity);

    static CameraDefaultInputCompEntity GetEmpty();

    /*  CreateComponentEntityByMap - CameraDefaultInputCompEntity
    "freezed",           isfreezed               = int
    "speed",             speed                   = float         (optional-default from config)
    "GlobalPosition",    globalPosition          = vec4.xyz      (optional-default  0, 0, 0)
    "GlobalDirection",   globalDirection         = vec4.xyz      (optional-default  0, 0, 1)
    "UpDirection",       upDirection             = vec4.xyz      (optional-default  0,-1, 0)
    */
    static CameraDefaultInputCompEntity CreateComponentEntityByMap(Entity in_entity, std::string entity_name, const CompEntityInitMap& in_map);

    void Init();
    void Update(CameraComp* cameraComp_ptr,
                std::chrono::duration<float> durationOfLastState);

    void AsyncInput(InputType input_type, void* struct_data,
                    std::chrono::duration<float> durationOfLastState);

    void Freeze();
    void Unfreeze();

private:
    void MoveCamera(float xRotation_rads, float yRotation_rads);
    void CalculateSnap(const std::chrono::duration<float> durationOfLastState);

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
};

#ifdef GAME_DLL
class CameraDefaultInputComp
    :public ComponentBaseWrappedClass<CameraDefaultInputCompEntity, static_cast<componentID>(componentIDenum::CameraDefaultInput), "CameraDefaultInput", sparse_set> {};
#else
#include "ECS/GeneralComponents/CameraDefaultInputComp.h"
#endif