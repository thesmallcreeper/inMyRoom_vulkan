#pragma once

#include "ECS/CompEntityBaseWrappedClass.h"

class LateNodeGlobalMatrixComp;
class LateNodeGlobalMatrixCompEntity;
#ifdef GAME_DLL
class LateNodeGlobalMatrixComp :
    public ComponentBaseWrappedClass<LateNodeGlobalMatrixCompEntity, static_cast<componentID>(componentIDenum::LateNodeGlobalMatrix), "LateNodeGlobalMatrix"> {};
#else
#include "ECS/GeneralComponents/LateNodeGlobalMatrixComp.h"
#endif

#include "ECS/GeneralCompEntities/NodeDataCompEntity.h"

class LateNodeGlobalMatrixCompEntity :
    public CompEntityBaseWrappedClass<LateNodeGlobalMatrixComp>
{
#ifndef GAME_DLL
public:
    LateNodeGlobalMatrixCompEntity(const Entity this_entity);

    static LateNodeGlobalMatrixCompEntity GetEmpty();

    /*  CreateComponentEntityByMap - LateNodeGlobalMatrix
    { "ParentEntity",       parent_entity           = Entity        
     ^"ParentName",         parent_name             = string        (optional)}
    "MatrixColumn0",        globalMatrix[0]         = vec4          (optional)
    "MatrixColumn1",        globalMatrix[1]         = vec4          (optional)
    "MatrixColumn2",        globalMatrix[2]         = vec4          (optional)
    "MatrixColumn3",        globalMatrix[3]         = vec4          (optional)
    */
    static LateNodeGlobalMatrixCompEntity CreateComponentEntityByMap(Entity in_entity, const CompEntityInitMap& in_map);
    static std::vector<std::pair<std::string, MapType>> GetComponentInitMapFields();

    void Init();
    void Update(const NodeDataCompEntity& this_nodeData,
                LateNodeGlobalMatrixComp* lateNodeGlobalMatrixComp_ptr);

#endif
public: // data
    glm::mat4x4 globalMatrix = glm::mat4x4(1.f);

    Entity parentEntity = 0;
};

