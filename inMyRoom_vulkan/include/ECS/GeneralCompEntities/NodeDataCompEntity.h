#pragma once

#include "ECS/CompEntityBaseWrappedClass.h"

#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"

class NodeDataComp;

class NodeDataCompEntity :
    public CompEntityBaseWrappedClass<NodeDataComp>
{
#ifndef GAME_DLL
public:
    NodeDataCompEntity(Entity this_entity);

    static NodeDataCompEntity GetEmpty();

    /*  CreateComponentEntityByMap - NodeData
        "LocalScale",           localScale.xyz          = vec4.xyz      (optional)
        "LocalRotation",        localRotation.xyzw      = vec4.xyzw     (optional)
        "LocalTranslation",     localTranslation.xyz    = vec4.xyz      (optional)
        "GlobalTranslation",    globalTranslation.xyz   = vec4.xyz      (optional)
    */
    static NodeDataCompEntity CreateComponentEntityByMap(Entity in_entity, std::string entity_name, const CompEntityInitMap& in_map);

    glm::mat4x4 GetGlobalMatrix(glm::mat4x4 parent_global_matrix) const;

    void Init();

    void Update();

#endif

public: // dll visible
    void LocalScale(glm::vec3 in_scale);
    void LocalRotate(glm::qua<float> in_rotation);
    void LocalTranslate(glm::vec3 in_translate);
    void GlobalTranslate(glm::vec3 in_translate);

public: // data
    glm::vec3 localScale_old = glm::vec3(1.f, 1.f, 1.f);;
    glm::qua<float> localRotation_old = glm::qua<float>(1.f, 0.f, 0.f, 0.f);
    glm::vec3 localTranslation_old = glm::vec3(0.f, 0.f, 0.f);
    glm::vec3 globalTranslation_old = glm::vec3(0.f, 0.f, 0.f);

    glm::vec3 localScale = glm::vec3(1.f, 1.f, 1.f);;
    glm::qua<float> localRotation = glm::qua<float>(1.f, 0.f, 0.f, 0.f);
    glm::vec3 localTranslation = glm::vec3(0.f, 0.f, 0.f);
    glm::vec3 globalTranslation = glm::vec3(0.f, 0.f, 0.f);
};

#ifdef GAME_DLL
class NodeDataComp :
    public ComponentBaseWrappedClass<NodeDataCompEntity, static_cast<componentID>(componentIDenum::NodeData), "NodeData", dense_set> {};
#else
#include "ECS/GeneralComponents/NodeDataComp.h"
#endif
