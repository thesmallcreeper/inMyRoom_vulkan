#pragma once

#include "ECS/ECStypes.h"

#include "glm/mat4x4.hpp"

class NodeGlobalMatrixComp;

class NodeGlobalMatrixCompEntity
{
public:
    NodeGlobalMatrixCompEntity(const Entity this_entity);
    ~NodeGlobalMatrixCompEntity();

    static NodeGlobalMatrixCompEntity GetEmpty();

    /*  CreateComponentEntityByMap - NodeGlobalMatrix
    { "ParentEntity",       parent_entity           = Entity        
     ^"ParentName",         parent_name             = string        (optional)}
    "MatrixColumn0",        globalMatrix[0]         = vec4          (optional)
    "MatrixColumn1",        globalMatrix[1]         = vec4          (optional)
    "MatrixColumn2",        globalMatrix[2]         = vec4          (optional)
    "MatrixColumn3",        globalMatrix[3]         = vec4          (optional)
    */
    static NodeGlobalMatrixCompEntity CreateComponentEntityByMap(const Entity in_entity, const CompEntityInitMap in_map);

    void Update(class PositionComp* const positionComp_ptr);

    void Init();

public: // data
    glm::mat4x4 globalMatrix = glm::mat4x4(1.f);

    Entity parentEntity = 0;
    Entity thisEntity;

private: // static variable
    friend class NodeGlobalMatrixComp;
    static NodeGlobalMatrixComp* nodeGlobalMatrixComp_ptr;
};

