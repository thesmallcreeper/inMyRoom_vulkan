#include "ECS/GeneralCompEntities/CameraDefaultInputCompEntity.h"

#include "ECS/ECSwrapper.h"

#include <glm/gtx/rotate_vector.hpp>

#ifndef GAME_DLL
#include "ECS/GeneralComponents/CameraDefaultInputComp.h"
#include "ECS/GeneralComponents/CameraComp.h"

CameraDefaultInputCompEntity::CameraDefaultInputCompEntity(const Entity this_entity)
    :CompEntityBase<CameraDefaultInputComp>(this_entity)
{
}

CameraDefaultInputCompEntity CameraDefaultInputCompEntity::GetEmpty()
{
    CameraDefaultInputCompEntity this_defaultCameraInputCompEntity(0);
    this_defaultCameraInputCompEntity.speed = GetComponentPtr()->default_speed;

    return this_defaultCameraInputCompEntity;
}

CameraDefaultInputCompEntity CameraDefaultInputCompEntity::CreateComponentEntityByMap(const Entity in_entity, const CompEntityInitMap& in_map)
{
    CameraDefaultInputCompEntity this_defaultCameraInputCompEntity(in_entity);

    // "freezed",           isfreezed               = bool      
    {
        auto search = in_map.intMap.find("freezed");
        assert(search != in_map.intMap.end());

        bool this_bool = static_cast<bool>(search->second);
        this_defaultCameraInputCompEntity.isFreezed = this_bool;
    }

    // "speed",             speed                   = float         (optional-default from config)
    {
        auto search = in_map.floatMap.find("speed");
        if (search != in_map.floatMap.end())
        {
            float speed = search->second;
            this_defaultCameraInputCompEntity.speed = speed;
        }
        else
            this_defaultCameraInputCompEntity.speed = GetComponentPtr()->default_speed;
    }

    // "GlobalPosition",    globalPosition          = vec4.xyz      (optional-default  0, 0, 0)
    {
        auto search = in_map.vec4Map.find("GlobalPosition");
        if (search != in_map.vec4Map.end())
        {
            glm::vec3 globalPosition = glm::vec3(search->second.x, search->second.y, search->second.z);
            this_defaultCameraInputCompEntity.globalPosition = globalPosition;
        }
    }

    // "GlobalDirection",   globalDirection         = vec4.xyz      (optional-default  0, 0, 1)
    {
        auto search = in_map.vec4Map.find("GlobalDirection");
        if (search != in_map.vec4Map.end())
        {
            glm::vec3 globalDirection = glm::vec3(search->second.x, search->second.y, search->second.z);
            this_defaultCameraInputCompEntity.globalDirection = globalDirection;
        }
    }

    // "UpDirection",       globalUp                = vec4.xyz      (optional-default  0,-1, 0)
    {
        auto search = in_map.vec4Map.find("UpDirection");
        if (search != in_map.vec4Map.end())
        {
            glm::vec3 globalUp = glm::vec3(search->second.x, search->second.y, search->second.z);
            this_defaultCameraInputCompEntity.globalDirection = globalUp;
        }
    }

    return this_defaultCameraInputCompEntity;
}

std::vector<std::pair<std::string, MapType>> CameraDefaultInputCompEntity::GetComponentInitMapFields()
{
    std::vector<std::pair<std::string, MapType>> return_pair;
    return_pair.emplace_back(std::make_pair("freezed",          MapType::bool_type));
    return_pair.emplace_back(std::make_pair("speed",            MapType::float_type));
    return_pair.emplace_back(std::make_pair("GlobalPosition",   MapType::vec3_type));
    return_pair.emplace_back(std::make_pair("GlobalDirection",  MapType::vec3_type));
    return_pair.emplace_back(std::make_pair("UpDirection",      MapType::vec3_type));

    return return_pair;
}

void CameraDefaultInputCompEntity::Init()
{
}

void CameraDefaultInputCompEntity::Update(CameraComp* const cameraComp_ptr,
                                          const std::chrono::duration<float> durationOfLastState)
{
    if (!isFreezed)
    {
        CalculateSnap(durationOfLastState);
        CameraCompEntity& this_cameraCompEntity = cameraComp_ptr->GetComponentEntity(thisEntity);

        this_cameraCompEntity.UpdateCameraViewMatrix(globalPosition,
                                                     globalDirection,
                                                     upDirection);
    }
}

void CameraDefaultInputCompEntity::AsyncInput(InputType input_type,
                                              void* struct_data,
                                              const std::chrono::duration<float> durationOfLastState)
{
    if (!isFreezed)
    {
        CalculateSnap(durationOfLastState);
        switch (input_type)
        {
            case InputType::MouseMove:
                {
                    InputMouse* mouse_input = reinterpret_cast<InputMouse*>(struct_data);
                    MoveCamera(mouse_input->x_axis, mouse_input->y_axis);
                }
                break;
            case InputType::MoveForward:
                movementState.movingForward = true;
                break;
            case InputType::MoveBackward:
                movementState.movingBackward = true;
                break;
            case InputType::MoveRight:
                movementState.movingRight = true;
                break;
            case InputType::MoveLeft:
                movementState.movingLeft = true;
                break;
            case InputType::MoveUp:
                movementState.movingUp = true;
                break;
            case InputType::MoveDown:
                movementState.movingDown = true;
                break;
            case InputType::StopMovingForward:
                movementState.movingForward = false;
                break;
            case InputType::StopMovingBackward:
                movementState.movingBackward = false;
                break;
            case InputType::StopMovingRight:
                movementState.movingRight = false;
                break;
            case InputType::StopMovingLeft:
                movementState.movingLeft = false;
                break;
            case InputType::StopMovingUp:
                movementState.movingUp = false;
                break;
            case InputType::StopMovingDown:
                movementState.movingDown = false;
                break;
            default:
                break;
        }
    }
}

void CameraDefaultInputCompEntity::Freeze()
{
    isFreezed = true;
}

void CameraDefaultInputCompEntity::Unfreeze()
{
    isFreezed = false;

    movementState.movingForward = false;
    movementState.movingBackward = false;
    movementState.movingRight = false;
    movementState.movingLeft = false;
    movementState.movingUp = false;
    movementState.movingDown = false;
}

void CameraDefaultInputCompEntity::MoveCamera(float xRotation_rads, float yRotation_rads)
{
    glm::vec3 new_global_direction = globalDirection;

    new_global_direction = glm::rotate(new_global_direction, yRotation_rads, glm::normalize(glm::cross(new_global_direction, upDirection)));
    {
        constexpr const float min_theta = 0.01f * glm::half_pi<float>();
        const float max_abs_directionY = glm::cos(min_theta);
        const float min_XZlength = glm::sin(min_theta);

        glm::vec2 old_XZorientation(globalDirection.x, globalDirection.z);
        glm::vec2 new_XZorientation(new_global_direction.x, new_global_direction.z);

        if (((glm::dot(old_XZorientation, new_XZorientation) < 0.0f) || (glm::dot(old_XZorientation, new_XZorientation) == 0.0f) || (glm::dot(old_XZorientation, new_XZorientation) == -0.0f)
             || (std::abs(new_global_direction.y) > max_abs_directionY)) && (std::abs(new_global_direction.y) > 0.5f))
        {
            glm::vec2 old_XZorientation_normalized = glm::normalize(old_XZorientation);
            new_global_direction = glm::vec3(min_XZlength * old_XZorientation_normalized.x, (globalDirection.y > 0.0f) ? max_abs_directionY : -max_abs_directionY, min_XZlength* old_XZorientation_normalized.y);
        }
    }
    new_global_direction = glm::rotate(new_global_direction, xRotation_rads, upDirection);

    globalDirection = glm::normalize(new_global_direction);
}

void CameraDefaultInputCompEntity::CalculateSnap(const std::chrono::duration<float> durationOfCurrentState)
{
    glm::vec3 position_delta_vector(0.0f, 0.0f, 0.0f);

    if (movementState.movingForward) position_delta_vector += globalDirection;
    if (movementState.movingBackward) position_delta_vector -= globalDirection;

    if (movementState.movingRight) position_delta_vector += glm::normalize(glm::cross(globalDirection, upDirection));
    if (movementState.movingLeft) position_delta_vector -= glm::normalize(glm::cross(globalDirection, upDirection));

    if (movementState.movingUp) position_delta_vector += upDirection;
    if (movementState.movingDown) position_delta_vector -= upDirection;

    if (position_delta_vector.x != 0.0 || position_delta_vector.y != 0.0 || position_delta_vector.z != 0.0)
        position_delta_vector = durationOfCurrentState.count() * speed * glm::normalize(position_delta_vector);

    globalPosition = globalPosition + position_delta_vector;
}

#endif