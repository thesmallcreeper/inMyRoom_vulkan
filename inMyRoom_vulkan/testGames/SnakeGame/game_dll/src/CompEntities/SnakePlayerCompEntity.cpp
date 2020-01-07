#include "CompEntities/SnakePlayerCompEntity.h"
#include "Components/SnakePlayerComp.h"

#include "ECS/ECSwrapper.h"
#include "ECS/GeneralCompEntities/NodeDataCompEntity.h"
#include "ECS/GeneralCompEntities/CameraCompEntity.h"
#include "ECS/GeneralCompEntities/AnimationComposerCompEntity.h"

#include <cassert>

SnakePlayerComp* SnakePlayerCompEntity::snakePlayerComp_ptr = nullptr;

SnakePlayerCompEntity::SnakePlayerCompEntity(const Entity this_entity)
    :thisEntity(this_entity)
{
}

SnakePlayerCompEntity::~SnakePlayerCompEntity()
{
}

SnakePlayerCompEntity SnakePlayerCompEntity::GetEmpty()
{
    SnakePlayerCompEntity this_snakePlayerEntity(0);

    return this_snakePlayerEntity;
}

SnakePlayerCompEntity SnakePlayerCompEntity::CreateComponentEntityByMap(const Entity in_entity, const CompEntityInitMap& in_map)
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
        this_snakePlayerEntity.animationComposerEntity = snakePlayerComp_ptr->GetECSwrapper()->GetEntitiesHandler()
                                                                            ->FindEntityByRelativeName(this_relative_node_name,
                                                                                                       this_snakePlayerEntity.thisEntity);
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

    return this_snakePlayerEntity;
}

std::vector<std::pair<std::string, MapType>> SnakePlayerCompEntity::GetComponentInitMapFields()
{
    std::vector<std::pair<std::string, MapType>> return_pair;
    return_pair.emplace_back(std::make_pair("Speed", MapType::float_type));
    return_pair.emplace_back(std::make_pair("RotationSpeed", MapType::float_type));
    return_pair.emplace_back(std::make_pair("AnimationComposer", MapType::string_type));
    return_pair.emplace_back(std::make_pair("CameraOffset", MapType::vec3_type));
    return_pair.emplace_back(std::make_pair("InitDirection", MapType::vec3_type));
    return_pair.emplace_back(std::make_pair("UpDirection", MapType::vec3_type));

    return return_pair;
}

void SnakePlayerCompEntity::Init()
{
    // bind snake's camera
    snakePlayerComp_ptr->GetECSwrapper()->GetEnginesExportedFunctions()->BindCameraEntity(thisEntity);

    // start animation
    ComponentBaseClass* animationComposerComp_bptr = snakePlayerComp_ptr->GetECSwrapper()->GetComponentByID(static_cast<componentID>(componentIDenum::AnimationComposer));
    AnimationComposerCompEntity* snake_animationComposer_compEntity_ptr = reinterpret_cast<AnimationComposerCompEntity*>(animationComposerComp_bptr->GetComponentEntity(animationComposerEntity));

    snake_animationComposer_compEntity_ptr->StartAnimation(true, 0.f);
}

void SnakePlayerCompEntity::Update(ComponentBaseClass* nodeDataComp_bptr, ComponentBaseClass* cameraComp_bptr, ComponentBaseClass* animationComposerComp_bptr, const std::chrono::duration<float> durationOfLastState)
{
    CalculateSnap(durationOfLastState);

    NodeDataCompEntity* this_nodeData_compEntity_ptr = reinterpret_cast<NodeDataCompEntity*>(nodeDataComp_bptr->GetComponentEntity(thisEntity));
    CameraCompEntity* this_camera_compEntity_ptr = reinterpret_cast<CameraCompEntity*>(cameraComp_bptr->GetComponentEntity(thisEntity));
    AnimationComposerCompEntity* snake_animationComposer_compEntity_ptr = reinterpret_cast<AnimationComposerCompEntity*>(animationComposerComp_bptr->GetComponentEntity(animationComposerEntity));

    {
        this_nodeData_compEntity_ptr->GlobalTranslate(delta_position);
        delta_position = glm::vec3(0.f, 0.f, 0.f);
        this_nodeData_compEntity_ptr->LocalRotate(delta_rotation);
        delta_rotation = glm::qua<float>(1.f, 0.f, 0.f, 0.f);
    }

    {
        glm::vec3 local_x = glm::normalize(globalDirection);
        glm::vec3 local_z = glm::normalize(glm::cross(local_x, -upDirection));
        glm::vec3 local_y = glm::normalize(glm::cross(local_z, local_x));

        glm::mat3 local_space_mat3= glm::mat3( local_x , local_y , local_z );

        glm::vec3 global_space_camera_offset = local_space_mat3 * cameraOffset;
        glm::vec3 global_space_camera = this_nodeData_compEntity_ptr->globalTranslation + global_space_camera_offset;

        this_camera_compEntity_ptr->UpdateCameraViewMatrix(global_space_camera,
                                                           globalDirection,
                                                           upDirection);
    }

    {
        if (movementState.movingForward)
            snake_animationComposer_compEntity_ptr->UnfreezeAnimation();
        else
            snake_animationComposer_compEntity_ptr->FreezeAnimation();
    }

}

void SnakePlayerCompEntity::AsyncInput(InputType input_type, void* struct_data, const std::chrono::duration<float> durationOfLastState)
{
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

        delta_position += position_delta_vector;
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

        delta_rotation = rotation_quat * delta_rotation;
    }

}
     
