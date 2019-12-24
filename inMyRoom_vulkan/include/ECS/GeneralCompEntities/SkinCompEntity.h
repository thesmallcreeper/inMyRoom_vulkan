#pragma once

#include "ECS/ECStypes.h"

#include "Meshes/SkinsOfMeshes.h"





class SkinComp;

class SkinCompEntity
{
public:
    SkinCompEntity(const Entity this_entity);
    ~SkinCompEntity();

    static SkinCompEntity GetEmpty();

    /*  CreateComponentEntityByMap - SkinCompEntity
        "InverseBindMatricesOffset",     inverseBindMatricesOffset     = int
        "JointRelativeName_X",           jointEntities[X]              = string    (name)
    */
    static SkinCompEntity CreateComponentEntityByMap(const Entity in_entity, const CompEntityInitMap in_map);
    static std::vector<std::pair<std::string, MapType>> GetComponentInitMapFields();

    void Init();
    void Update(class NodeGlobalMatrixComp* const nodeGlobalMatrixComp_ptr, SkinsOfMeshes* skinsOfMeshes_ptr);

public: // data
    std::vector<Entity> jointEntities;

    uint32_t inverseBindMatricesOffset;
    uint32_t lastNodesMatricesOffset;

    Entity thisEntity;

private: // static_variable
    friend class SkinComp;
    static SkinComp* skinComp_ptr;
};