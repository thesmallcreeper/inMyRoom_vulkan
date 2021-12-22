#pragma once

#include "ECS/CompEntityBaseWrappedClass.h"

class DynamicMeshComp;

#include "ECS/GeneralCompEntities/ModelDrawCompEntity.h"
#include "ECS/GeneralCompEntities/LateNodeGlobalMatrixCompEntity.h"

class DynamicMeshCompEntity :
    public CompEntityBaseWrappedClass<DynamicMeshComp>
{
#ifndef GAME_DLL
public:
    DynamicMeshCompEntity(Entity this_entity);

    static DynamicMeshCompEntity GetEmpty();

    /*  CreateComponentEntityByMap - DynamicMeshCompEntity
        "InverseBindMatricesOffset",     inverseBindMatricesOffset     = int
        "JointRelativeName_X",           jointEntities[X]              = string    (name)
        "DefaultWeight_X",               morphTargetsWeights[X]        = float
    */
    static DynamicMeshCompEntity CreateComponentEntityByMap(Entity in_entity, std::string entity_name, const CompEntityInitMap& in_map);

    void Update( ModelDrawComp* modelDrawComp_ptr, class DynamicMeshes* dynamicMeshes_ptr, class MeshesOfNodes* meshesOfNodes_ptr);
    void ToBeRemovedCallBack(class DynamicMeshes* dynamicMeshes_ptr);

#endif
    bool IsSkin() const {return jointRelativeEntities.size();}
    bool HasMorphTargets() const {return morphTargetsWeights.size();}
public: // data
    // skin
    std::vector<Entity> jointRelativeEntities;
    uint32_t inverseBindMatricesOffset = -1;

    // morph targets
    std::vector<float> morphTargetsWeights;

    // dynamic mesh index
    size_t dynamicMeshIndex = -1;
    bool toBeRemoved = false;
};

#ifdef GAME_DLL
class DynamicMeshComp
    :public ComponentBaseWrappedClass<DynamicMeshCompEntity, static_cast<componentID>(componentIDenum::DynamicMesh), "DynamicMesh", sparse_set> {};
#else
#include "ECS/GeneralComponents/DynamicMeshComp.h"
#endif