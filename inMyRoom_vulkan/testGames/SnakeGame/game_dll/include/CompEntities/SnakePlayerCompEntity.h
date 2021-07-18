#pragma once

#include "ECS/CompEntityBaseWrappedClass.h"
#include <chrono>

class SnakePlayerComp;
#include "ECS/GeneralCompEntities/NodeDataCompEntity.h"
#include "ECS/GeneralCompEntities/CameraCompEntity.h"
#include "ECS/GeneralCompEntities/CameraDefaultInputCompEntity.h"
#include "ECS/GeneralCompEntities/AnimationComposerCompEntity.h"

class SnakePlayerCompEntity:
    public CompEntityBaseWrappedClass<SnakePlayerComp>
{
public:
    SnakePlayerCompEntity(const Entity this_entity);

    static SnakePlayerCompEntity GetEmpty();

    /*  CreateComponentEntityByMap - SnakePlayerCompEntity
        "Speed",                    speed                       = float
        "RotationSpeed",            rotationSpeed               = float
        "AnimationComposer",        animationComposer_entity    = string    (relative path)
        "CameraOffset",             cameraOffset                = vec4.xyz
        "InitDirection",            globalDirection             = vec4.xyz  (optional-default -1, 0, 0)
        "UpDirection",              globalUp                    = vec4.xyz  (optional-default  0,-1, 0)
        "IsHumanPlayer",            isHumanPlayer               = bool      (optional-default false)
    */
    static SnakePlayerCompEntity CreateComponentEntityByMap(Entity in_entity, const std::string& entity_name, const CompEntityInitMap& in_map);

    void Init();
    void Update(NodeDataComp* nodeDataComp_ptr,
                CameraComp* cameraComp_ptr,
                AnimationActorComp* animationActorComp_ptr,
                AnimationComposerComp* animationComposerComp_ptr,
                const std::chrono::duration<float> update_deltaTime,
                const std::chrono::duration<float> async_durationOfLastState);

    void AsyncInput(InputType input_type, void* struct_data, const std::chrono::duration<float> durationOfLastState);

    void CollisionUpdate(NodeDataComp* nodeDataComp_ptr,
                         CameraComp* cameraComp_ptr,
                         CameraDefaultInputComp* cameraDefaultInputComp_ptr,
                         AnimationActorComp* animationActorComp_ptr,
                         AnimationComposerComp* animationComposerComp_ptr,
                         const CollisionCallbackData& this_collisionCallbackData);

private:
    void CalculateSnap(const std::chrono::duration<float> durationOfLastState);

public: //data
    struct
    {
        bool movingForward = false;
        bool movingRight = false;
        bool movingLeft = false;
        bool shouldJump = false;
    } movementState;

    glm::vec3 delta_position_input = glm::vec3(0.f, 0.f, 0.f);
    glm::qua<float> delta_rotation_input = glm::qua<float>(1.f, 0.f, 0.f, 0.f);

    glm::vec3 globalDirection = glm::vec3(0.f, 0.f, -1.f);

    float speed;
    float rotationSpeed;
    glm::vec3 upDirection = glm::vec3(0.f, -1.f, 0.f);

    glm::vec3 cameraOffset;

    float gravitySpeed = 0.f;
    float gravityAcceleration = 7.f;

    bool isHumanPlayer = false;
    bool isGoingToDelete = false;

    bool shouldStartAnimation = true;

    Entity animationComposerRelativeEntity;
};

#include "Components/SnakePlayerComp.h"