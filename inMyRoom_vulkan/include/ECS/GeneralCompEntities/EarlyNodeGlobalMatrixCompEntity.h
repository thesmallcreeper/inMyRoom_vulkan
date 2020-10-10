#pragma once

#include "ECS/CompEntityBase.h"

#include "ECS/GeneralCompEntities/NodeDataCompEntity.h"

#ifdef GAME_DLL
class EarlyNodeGlobalMatrixCompEntity;
class EarlyNodeGlobalMatrixComp :
    public ComponentBaseWrappedClass<EarlyNodeGlobalMatrixCompEntity, static_cast<componentID>(componentIDenum::EarlyNodeGlobalMatrix), "EarlyNodeGlobalMatrix"> {};
#else
class EarlyNodeGlobalMatrixComp;
#endif

class EarlyNodeGlobalMatrixCompEntity :
    public CompEntityBase<EarlyNodeGlobalMatrixComp>
{
#ifndef GAME_DLL
public:
    EarlyNodeGlobalMatrixCompEntity(Entity this_entity);

    static EarlyNodeGlobalMatrixCompEntity GetEmpty();

    /*  CreateComponentEntityByMap - EarlyNodeGlobalMatrix
    { "ParentEntity",       parent_entity           = Entity        
     ^"ParentName",         parent_name             = string        (optional)}
    "MatrixColumn0",        globalMatrix[0]         = vec4          (optional)
    "MatrixColumn1",        globalMatrix[1]         = vec4          (optional)
    "MatrixColumn2",        globalMatrix[2]         = vec4          (optional)
    "MatrixColumn3",        globalMatrix[3]         = vec4          (optional)
    */
    static EarlyNodeGlobalMatrixCompEntity CreateComponentEntityByMap(Entity in_entity, const CompEntityInitMap& in_map);
    static std::vector<std::pair<std::string, MapType>> GetComponentInitMapFields();

    void Init();
    void Update(NodeDataComp* positionComp_ptr,
                EarlyNodeGlobalMatrixComp* earlyNodeGlobalMatrixComp_ptr);

#endif
public: // data
    glm::mat4x4 globalMatrix = glm::mat4x4(1.f);

    Entity parentEntity = 0;
};

