#pragma once

#include "ECS/CompEntityBaseWrappedClass.h"

class LateNodeGlobalMatrixComp;

#include "ECS/GeneralCompEntities/NodeDataCompEntity.h"

class LateNodeGlobalMatrixCompEntity :
    public CompEntityBaseWrappedClass<LateNodeGlobalMatrixComp>
{
#ifndef GAME_DLL
public:
    LateNodeGlobalMatrixCompEntity(Entity this_entity);

    static LateNodeGlobalMatrixCompEntity GetEmpty();

    /*  CreateComponentEntityByMap - LateNodeGlobalMatrix
    { "ParentEntity",       parent_entity           = Entity        
     ^"ParentName",         parent_name             = string        (optional)}
    "MatrixColumn0",        globalMatrix[0]         = vec4          (optional)
    "MatrixColumn1",        globalMatrix[1]         = vec4          (optional)
    "MatrixColumn2",        globalMatrix[2]         = vec4          (optional)
    "MatrixColumn3",        globalMatrix[3]         = vec4          (optional)
    */
    static LateNodeGlobalMatrixCompEntity CreateComponentEntityByMap(Entity in_entity, std::string entity_name, const CompEntityInitMap& in_map);

    void Update(EntitiesHandler* entities_handler_ptr,
                NodeDataComp* nodeDataComp_ptr,
                LateNodeGlobalMatrixComp* lateNodeGlobalMatrixComp_ptr);

#endif
public: // data
    glm::mat4x4 globalMatrix = glm::mat4x4(1.f);
};

#ifdef GAME_DLL
class LateNodeGlobalMatrixComp :
    public ComponentBaseWrappedClass<LateNodeGlobalMatrixCompEntity, static_cast<componentID>(componentIDenum::LateNodeGlobalMatrix), "LateNodeGlobalMatrix", dense_set> {};
#else
#include "ECS/GeneralComponents/LateNodeGlobalMatrixComp.h"
#endif
