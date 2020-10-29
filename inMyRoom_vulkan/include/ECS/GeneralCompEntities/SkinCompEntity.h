#pragma once

#include "ECS/CompEntityBaseWrappedClass.h"

class SkinComp;

#include "ECS/GeneralCompEntities/LateNodeGlobalMatrixCompEntity.h"

class SkinCompEntity :
    public CompEntityBaseWrappedClass<SkinComp>
{
#ifndef GAME_DLL
public:
    SkinCompEntity(Entity this_entity);

    static SkinCompEntity GetEmpty();

    /*  CreateComponentEntityByMap - SkinCompEntity
        "InverseBindMatricesOffset",     inverseBindMatricesOffset     = int
        "JointRelativeName_X",           jointEntities[X]              = string    (name)
    */
    static SkinCompEntity CreateComponentEntityByMap(Entity in_entity, std::string entity_name, const CompEntityInitMap& in_map);

    void Init();
    void Update(LateNodeGlobalMatrixComp* nodeGlobalMatrixComp_ptr,
                class SkinsOfMeshes* skinsOfMeshes_ptr);

#endif
public: // data
    std::vector<Entity> jointRelativeEntities;

    uint32_t inverseBindMatricesOffset;
    uint32_t lastNodesMatricesOffset;
};

#ifdef GAME_DLL
class SkinComp 
    :public ComponentBaseWrappedClass<SkinCompEntity, static_cast<componentID>(componentIDenum::Skin), "Skin", sparse_set> {};
#else
#include "ECS/GeneralComponents/SkinComp.h"
#endif