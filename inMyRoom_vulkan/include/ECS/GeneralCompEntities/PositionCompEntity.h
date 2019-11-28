#pragma once

#include "ECS/ECStypes.h"

#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"

class PositionComp;

class PositionCompEntity
{
public:
    PositionCompEntity(const Entity this_entity);
    ~PositionCompEntity();

    static PositionCompEntity GetEmpty();

    /*  CreateComponentEntityByMap - Position
        "LocalScale",           localScale.xyz          = vec4.xyz
        "LocalRotation",        localRotation.xyzw      = vec4.xyzw
        "LocalTranslation",     localTranslation.xyz    = vec4.xyz
        "GlobalTranslation",    globalTranslation.xyz   = vec4.xyz      (optional)
    */
    static PositionCompEntity CreateComponentEntityByMap(const Entity in_entity, const ComponentEntityInitMap in_map);

    void LocalScale(const glm::vec3 in_scale);
    void LocalRotate(const glm::qua<float> in_rotation);
    void LocalTranslate(const glm::vec3 in_translate);
    void GlobalTranslate(const glm::vec3 in_translate);

    glm::mat4x4 GetGlobalMatrix(const glm::mat4x4 parent_global_matrix);

public: // data
    glm::vec3 localScale = glm::vec3(1.f, 1.f, 1.f);;

    glm::qua<float> localRotation = glm::qua<float>(1.f, 0.f, 0.f, 0.f);

    glm::vec3 localTranslation = glm::vec3(0.f, 0.f, 0.f);;
    glm::vec3 globalTranslation = glm::vec3(0.f, 0.f, 0.f);

    Entity thisEntity;

private: // static_variable
    friend class PositionComp;
    static PositionComp* positionComp_ptr;
};

