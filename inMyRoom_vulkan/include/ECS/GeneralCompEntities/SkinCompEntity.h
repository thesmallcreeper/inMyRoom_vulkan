#pragma once

#include "ECS/CompEntityBase.h"

#include "ECS/GeneralCompEntities/LateNodeGlobalMatrixCompEntity.h"

#ifdef GAME_DLL
class SkinCompEntity;
class SkinComp 
    :public ComponentBaseWrappedClass<SkinCompEntity, static_cast<componentID>(componentIDenum::Skin), "Skin"> {};
#else
class SkinComp;
#endif

class SkinCompEntity :
    public CompEntityBase<SkinComp>
{
#ifndef GAME_DLL
public:
    SkinCompEntity(Entity this_entity);

    static SkinCompEntity GetEmpty();

    /*  CreateComponentEntityByMap - SkinCompEntity
        "InverseBindMatricesOffset",     inverseBindMatricesOffset     = int
        "JointRelativeName_X",           jointEntities[X]              = string    (name)
    */
    static SkinCompEntity CreateComponentEntityByMap(Entity in_entity, const CompEntityInitMap& in_map);
    static std::vector<std::pair<std::string, MapType>> GetComponentInitMapFields();

    void Init();
    void Update(LateNodeGlobalMatrixComp* nodeGlobalMatrixComp_ptr,
                class SkinsOfMeshes* skinsOfMeshes_ptr);

#endif
public: // data
    std::vector<Entity> jointEntities;

    uint32_t inverseBindMatricesOffset;
    uint32_t lastNodesMatricesOffset;
};