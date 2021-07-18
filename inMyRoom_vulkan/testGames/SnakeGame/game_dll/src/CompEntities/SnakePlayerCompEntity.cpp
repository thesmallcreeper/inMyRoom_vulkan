#include "CompEntities/SnakePlayerCompEntity.h"
#include "Components/SnakePlayerComp.h"

#include "ECS/ECSwrapper.h"
#include "ECS/ComponentsIDsEnum.h"
#include "ECS/GeneralCompEntities/NodeDataCompEntity.h"
#include "ECS/GeneralCompEntities/CameraCompEntity.h"
#include "ECS/GeneralCompEntities/AnimationComposerCompEntity.h"
#include "ECS/GeneralCompEntities/CameraDefaultInputCompEntity.h"

#include <cassert>
#include <iostream>

SnakePlayerCompEntity::SnakePlayerCompEntity(const Entity this_entity)
    :CompEntityBaseWrappedClass<SnakePlayerComp>(this_entity)
{
}

SnakePlayerCompEntity SnakePlayerCompEntity::GetEmpty()
{
    SnakePlayerCompEntity this_snakePlayerEntity(0);

    return this_snakePlayerEntity;
}

SnakePlayerCompEntity SnakePlayerCompEntity::CreateComponentEntityByMap(Entity in_entity, 
                                                                        const std::string& entity_name,
                                                                        const CompEntityInitMap& in_map)
{
    SnakePlayerCompEntity this_snakePlayerEntity(in_entity);

    //  "Speed",                    speed                       = float
    {
        auto search = in_map.floatMap.find("Speed");
        assert(search != in_map.floatMap.end());

        float this_float = static_cast<float>(search->second);
        this_snakePlayerEntity.speed = this_float;
    }

    // "RotationSpeed",             rotationSpeed               = float
    {
        auto search = in_map.floatMap.find("RotationSpeed");
        assert(search != in_map.floatMap.end());

        float this_float = static_cast<float>(search->second);
        this_snakePlayerEntity.rotationSpeed = this_float;
    }

    //  "AnimationComposer"         animationComposer_entity    = string    (relative path)
    {
        auto search = in_map.stringMap.find("AnimationComposer");
        assert(search != in_map.stringMap.end());

        std::string this_relative_node_name = search->second;
        this_snakePlayerEntity.animationComposerRelativeEntity = GetComponentPtr()->GetECSwrapper()
                                                                                  ->GetRelativeEntityOffset(entity_name,
                                                                                                            this_relative_node_name);
    }

    //  "CameraOffset",             cameraOffset                = vec4.xyz
    {
        auto search = in_map.vec4Map.find("CameraOffset");
        assert(search != in_map.vec4Map.end());

        glm::vec4 this_vec4 = search->second;
        this_snakePlayerEntity.cameraOffset = glm::vec3(this_vec4.x, this_vec4.y, this_vec4.z);
    }

    //  "InitDirection",            globalDirection                   = vec4.xyz  (optional-default -1, 0, 0)
    {
        auto search = in_map.vec4Map.find("InitDirection");
        if (search != in_map.vec4Map.end())
        {
            glm::vec4 this_vec4 = search->second;
            this_snakePlayerEntity.globalDirection = glm::vec3(this_vec4.x, this_vec4.y, this_vec4.z);
        }
    }

    //  "UpDirection",              globalUp                    = vec4.xyz  (optional-default  0,-1, 0)
    {
        auto search = in_map.vec4Map.find("UpDirection");
        if (search != in_map.vec4Map.end())
        {
            glm::vec4 this_vec4 = search->second;
            this_snakePlayerEntity.upDirection = glm::vec3(this_vec4.x, this_vec4.y, this_vec4.z);
        }
    }

    // "IsHumanPlayer",             isHumanPlayer               = bool    (optional - default false)
    {
        auto search = in_map.intMap.find("IsHumanPlayer");
        if (search != in_map.intMap.end())
        {
            int this_int = search->second;
            this_snakePlayerEntity.isHumanPlayer = static_cast<bool>(this_int);
        }
    }

    return this_snakePlayerEntity;
}

void SnakePlayerCompEntity::Init()
{
    if (isHumanPlayer)
    {
        // bind snake's camera
        GetComponentPtr()->GetECSwrapper()->GetEnginesExportedFunctions()->BindCameraEntity(thisEntity);
    }
    else
    {
        movementState.movingLeft = false;
        movementState.movingRight = true;
    }


}

void SnakePlayerCompEntity::Update(NodeDataComp* nodeDataComp_ptr,
                                   CameraComp* cameraComp_ptr,
                                   AnimationActorComp* animationActorComp_ptr,
                                   AnimationComposerComp* animationComposerComp_ptr,
                                   const std::chrono::duration<float> update_deltaTime,
                                   const std::chrono::duration<float> async_durationOfLastState)
{
    // start animation
    if(shouldStartAnimation)
    {
        AnimationComposerCompEntity& snake_animationComposer_compEntity_ptr = animationComposerComp_ptr->GetComponentEntity(animationComposerRelativeEntity + thisEntity);

        snake_animationComposer_compEntity_ptr.StartAnimation(nodeDataComp_ptr,
                                                              animationActorComp_ptr,
                                                              true, 0.f);

        shouldStartAnimation = false;
    }

    if (isHumanPlayer)
        CalculateSnap(async_durationOfLastState);
    else
        CalculateSnap(update_deltaTime);

    NodeDataCompEntity& this_nodeData_compEntity = nodeDataComp_ptr->GetComponentEntity(thisEntity);
    CameraCompEntity& this_camera_compEntity = cameraComp_ptr->GetComponentEntity(thisEntity);
    AnimationComposerCompEntity& snake_animationComposer_compEntity = animationComposerComp_ptr->GetComponentEntity(animationComposerRelativeEntity + thisEntity);

    {
        this_nodeData_compEntity.GlobalTranslate(delta_position_input);
        delta_position_input = glm::vec3(0.f, 0.f, 0.f);
        this_nodeData_compEntity.LocalRotate(delta_rotation_input);
        delta_rotation_input = glm::qua<float>(1.f, 0.f, 0.f, 0.f);
    }

    {
        if (movementState.movingForward)
            snake_animationComposer_compEntity.UnfreezeAnimation(animationActorComp_ptr);
        else
            snake_animationComposer_compEntity.FreezeAnimation(animationActorComp_ptr);
    }

    {
        if (movementState.shouldJump)
        {
            if (gravitySpeed == 0.f)
            {
                gravitySpeed = -10.f;
            }

            movementState.shouldJump = false;
        }

        glm::vec3 gravity_distance = (gravitySpeed * update_deltaTime.count() + 0.5f * gravityAcceleration * update_deltaTime.count() * update_deltaTime.count()) * glm::vec3(0.f, 1.f, 0.f);
        this_nodeData_compEntity.GlobalTranslate(gravity_distance);

        gravitySpeed += gravityAcceleration * update_deltaTime.count();
        if (gravitySpeed > 7.5f)
            gravitySpeed = 7.5f;
    }

    {
        glm::vec3 local_x = glm::normalize(globalDirection);
        glm::vec3 local_z = glm::normalize(glm::cross(local_x, -upDirection));
        glm::vec3 local_y = glm::normalize(glm::cross(local_z, local_x));

        glm::mat3 local_space_mat3= glm::mat3( local_x , local_y , local_z );

        glm::vec3 global_space_camera_offset = local_space_mat3 * cameraOffset;
        glm::vec3 global_space_camera = this_nodeData_compEntity.globalTranslation + global_space_camera_offset;

        this_camera_compEntity.UpdateCameraViewMatrix(global_space_camera,
                                                           globalDirection,
                                                           upDirection);
    }

    if (!isHumanPlayer)
    {
        movementState.movingForward = true;

        if (static_cast<float>(std::rand() % 100000) < 30000.f * update_deltaTime.count())
        {
            movementState.movingLeft = !movementState.movingLeft;
            movementState.movingRight = !movementState.movingRight;
        }

        if (static_cast<float>(std::rand() % 100000) < 5000.f * update_deltaTime.count())
        {
            movementState.shouldJump = true;
        }
    }
}

void SnakePlayerCompEntity::CollisionUpdate(NodeDataComp* nodeDataComp_ptr,
                                            CameraComp* cameraComp_ptr,
                                            CameraDefaultInputComp* cameraDefaultInputComp_ptr,
                                            AnimationActorComp* animationActorComp_ptr,
                                            AnimationComposerComp* animationComposerComp_ptr,
                                            const CollisionCallbackData& this_collisionCallbackData)
{
    NodeDataCompEntity& this_nodeData_compEntity_ptr = nodeDataComp_ptr->GetComponentEntity(thisEntity);
    CameraCompEntity& this_camera_compEntity_ptr = cameraComp_ptr->GetComponentEntity(thisEntity);
    AnimationComposerCompEntity& snake_animationComposer_compEntity_ptr = animationComposerComp_ptr->GetComponentEntity(animationComposerRelativeEntity + thisEntity);



    {
        glm::vec3 delta_vector = this_collisionCallbackData.deltaVector;
        if (glm::dot(delta_vector, glm::vec3(0.f, 1.f, 0.f)) > 0.f)
        {
            delta_vector -= glm::vec3(0.f, 1.f, 0.f) * glm::dot(delta_vector, glm::vec3(0.f, 1.f, 0.f));
        }

        this_nodeData_compEntity_ptr.GlobalTranslate(delta_vector);
    }

    {
        glm::vec3 local_x = glm::normalize(globalDirection);
        glm::vec3 local_z = glm::normalize(glm::cross(local_x, -upDirection));
        glm::vec3 local_y = glm::normalize(glm::cross(local_z, local_x));

        glm::mat3 local_space_mat3 = glm::mat3(local_x, local_y, local_z);

        glm::vec3 global_space_camera_offset = local_space_mat3 * cameraOffset;
        glm::vec3 global_space_camera = this_nodeData_compEntity_ptr.globalTranslation + global_space_camera_offset;

        this_camera_compEntity_ptr.UpdateCameraViewMatrix(global_space_camera,
                                                           globalDirection,
                                                           upDirection);
    }

    if (glm::dot(glm::normalize(this_collisionCallbackData.deltaVector), glm::vec3(0.f, -1.f, 0.f)) > 0.866f)
    {
        gravitySpeed = 0.f;
    }

    if (isHumanPlayer && !isGoingToDelete)
    {
        {
            std::string collided_with_name = GetComponentPtr()->GetECSwrapper()->GetEntitiesHandler()->GetEntityName(this_collisionCallbackData.collideWithEntity).second;
            auto search = collided_with_name.find("snake");
            if (search != std::string::npos)
            {
                Entity default_camera_entity = GetComponentPtr()->GetECSwrapper()->GetEntitiesHandler()->FindEntityByPath("_defaultCamera", "_defaultCamera");
                CameraDefaultInputCompEntity& default_camera_input = cameraDefaultInputComp_ptr->GetComponentEntity(default_camera_entity);
                {
                    glm::vec3 local_x = glm::normalize(globalDirection);
                    glm::vec3 local_z = glm::normalize(glm::cross(local_x, -upDirection));
                    glm::vec3 local_y = glm::normalize(glm::cross(local_z, local_x));

                    glm::mat3 local_space_mat3 = glm::mat3(local_x, local_y, local_z);

                    glm::vec3 global_space_camera_offset = local_space_mat3 * cameraOffset;
                    glm::vec3 global_space_camera = this_nodeData_compEntity_ptr.globalTranslation + global_space_camera_offset;

                    default_camera_input.globalPosition = global_space_camera;
                    default_camera_input.globalDirection = globalDirection;
                }

                GetComponentPtr()->GetECSwrapper()->GetEnginesExportedFunctions()->BindCameraEntity(default_camera_entity);

                Entity parent_entity = GetComponentPtr()->GetECSwrapper()->GetEntitiesHandler()->GetParentOfEntity(thisEntity);

                InstanceInfo* parent_instance_info_ptr = GetComponentPtr()->GetECSwrapper()->GetEntitiesHandler()->GetInstanceInfo(parent_entity);
                GetComponentPtr()->GetECSwrapper()->RemoveInstance(parent_instance_info_ptr);

                isGoingToDelete = true;
            }
        }
        {
            std::string collided_with_name = GetComponentPtr()->GetECSwrapper()->GetEntitiesHandler()->GetEntityName(this_collisionCallbackData.collideWithEntity).second;
            auto search = collided_with_name.find("apple");
            if (search != std::string::npos)
            {
                this_nodeData_compEntity_ptr.LocalScale(glm::vec3(1.5f, 1.5f, 1.5f));

                Entity apple_main_entity = this_collisionCallbackData.collideWithEntity;

                InstanceInfo* apple_instance_info_ptr = GetComponentPtr()->GetECSwrapper()->GetEntitiesHandler()->GetInstanceInfo(apple_main_entity);
                GetComponentPtr()->GetECSwrapper()->RemoveInstance(apple_instance_info_ptr);
            }
        }
    }
}

void SnakePlayerCompEntity::AsyncInput(InputType input_type, void* struct_data, const std::chrono::duration<float> durationOfLastState)
{
    if (!isHumanPlayer) return;

    CalculateSnap(durationOfLastState);

    switch (input_type)
    {
        case InputType::MoveForward:
            movementState.movingForward = true;
            break;
        case InputType::MoveRight:
            movementState.movingRight = true;
            break;
        case InputType::MoveLeft:
            movementState.movingLeft = true;
            break;
        case InputType::StopMovingForward:
            movementState.movingForward = false;
            break;
        case InputType::StopMovingRight:
            movementState.movingRight = false;
            break;
        case InputType::StopMovingLeft:
            movementState.movingLeft = false;
            break;
        case InputType::MoveUp:
            movementState.shouldJump = true;
            break;
        default:
            break;
    }

}

void SnakePlayerCompEntity::CalculateSnap(const std::chrono::duration<float> durationOfLastState)
{
    {
        glm::vec3 position_delta_vector(0.0f, 0.0f, 0.0f);

         if (movementState.movingForward)
            position_delta_vector = durationOfLastState.count() * speed * globalDirection;

        delta_position_input += position_delta_vector;
    }

    {
        float rads_rotation = 0.f;

        if (movementState.movingLeft)
            rads_rotation += durationOfLastState.count() * rotationSpeed;
        if (movementState.movingRight)
            rads_rotation -= durationOfLastState.count() * rotationSpeed;

        glm::qua<float> rotation_quat = glm::angleAxis(rads_rotation, upDirection);

        glm::vec4 globalDirection_vec4 = glm::mat4_cast<float>(rotation_quat) * glm::vec4(globalDirection, 0.f);
        globalDirection = glm::vec3(globalDirection_vec4.x, globalDirection_vec4.y, globalDirection_vec4.z);

        delta_rotation_input = rotation_quat * delta_rotation_input;
    }

}
     
