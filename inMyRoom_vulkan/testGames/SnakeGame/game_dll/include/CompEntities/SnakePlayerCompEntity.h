#pragma once

#include "ECS/ECStypes.h"

#include <chrono>


class SnakePlayerComp;

class SnakePlayerCompEntity
{
public:
    SnakePlayerCompEntity(const Entity this_entity);
    ~SnakePlayerCompEntity();

    static SnakePlayerCompEntity GetEmpty();

    /*  CreateComponentEntityByMap - SnakePlayerCompEntity
        "Speed",                    speed                       = float
        "RotationSpeed",            rotationSpeed               = float
        "AnimationComposer"         animationComposer_entity    = string    (relative path)
        "InitDirection",            globalDirection                   = vec4.xyz  (optional-default -1, 0, 0)
        "UpDirection",              globalUp                    = vec4.xyz  (optional-default  0,-1, 0)
    */
    static SnakePlayerCompEntity CreateComponentEntityByMap(const Entity in_entity, const CompEntityInitMap& in_map);
    static std::vector<std::pair<std::string, MapType>> GetComponentInitMapFields();

    void Init();
    void Update(class ComponentBaseClass* nodeDataComp_bptr,
                class ComponentBaseClass* cameraComp_bptr,
                class ComponentBaseClass* animationComposerComp_bptr,
                const std::chrono::duration<float> durationOfLastState);
    void AsyncInput(InputType input_type, void* struct_data, const std::chrono::duration<float> durationOfLastState);

private:
    void CalculateSnap(const std::chrono::duration<float> durationOfLastState);

private: // static_variable
    friend class SnakePlayerComp;
    static SnakePlayerComp* snakePlayerComp_ptr;

public: //data
    struct
    {
        bool movingForward = false;
        bool movingRight = false;
        bool movingLeft = false;
    } movementState;

    glm::vec3 delta_position = glm::vec3(0.f, 0.f, 0.f);
    glm::qua<float> delta_rotation = glm::qua<float>(1.f, 0.f, 0.f, 0.f);

    glm::vec3 globalDirection = glm::vec3(0.f, 0.f, -1.f);

    float speed;
    float rotationSpeed;
    glm::vec3 upDirection = glm::vec3(0.f, -1.f, 0.f);

    Entity animationComposerEntity;

    Entity thisEntity;
};