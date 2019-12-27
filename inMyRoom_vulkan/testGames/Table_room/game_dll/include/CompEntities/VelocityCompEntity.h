#pragma once

#include <chrono>

#include "ECS/ECStypes.h"

#include "glm/vec3.hpp"

class VelocityComp;

class VelocityCompEntity
{
public:
    VelocityCompEntity(const Entity this_entity);
    ~VelocityCompEntity();

    static VelocityCompEntity GetEmpty();

    /*  CreateComponentEntityByMap - Velocity
        "Velocity",           localScale.xyz          = vec4.xyz      (optional)
    */
    static VelocityCompEntity CreateComponentEntityByMap(const Entity in_entity, const CompEntityInitMap in_map);
    static std::vector<std::pair<std::string, MapType>> GetComponentInitMapFields();

    void Update(class ComponentBaseClass* positionComp_bptr, std::chrono::duration<float> deltaTime);

    void Init();

private: // static_variable
    friend class VelocityComp;
    static VelocityComp* velocityComp_ptr;

public: // data
    glm::vec3 velocity = glm::vec3(0.f, 0.f, 0.f);

    Entity thisEntity;
};