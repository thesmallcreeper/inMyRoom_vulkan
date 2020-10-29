#pragma once

#include "ECS/CompEntityBaseWrappedClass.h"

class EarlyNodeGlobalMatrixComp;

#include "ECS/GeneralCompEntities/NodeDataCompEntity.h"

class EarlyNodeGlobalMatrixCompEntity :
    public CompEntityBaseWrappedClass<EarlyNodeGlobalMatrixComp>
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
    static EarlyNodeGlobalMatrixCompEntity CreateComponentEntityByMap(Entity in_entity, std::string entity_name, const CompEntityInitMap& in_map);

    void Update(EntitiesHandler* entities_handler_ptr,
                NodeDataComp* nodeDataComp_ptr,
                EarlyNodeGlobalMatrixComp* earlyNodeGlobalMatrixComp_ptr);

#endif
public: // data
    glm::mat4x4 globalMatrix = glm::mat4x4(1.f);
};

#ifdef GAME_DLL
class EarlyNodeGlobalMatrixComp :
    public ComponentBaseWrappedClass<EarlyNodeGlobalMatrixCompEntity, static_cast<componentID>(componentIDenum::EarlyNodeGlobalMatrix), "EarlyNodeGlobalMatrix", dense_set> {};
#else
#include "ECS/GeneralComponents/EarlyNodeGlobalMatrixComp.h"
#endif
