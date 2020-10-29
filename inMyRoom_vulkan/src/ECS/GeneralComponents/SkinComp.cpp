#include "ECS/GeneralComponents/SkinComp.h"

#include "ECS/ECSwrapper.h"

SkinComp::SkinComp(ECSwrapper* const in_ecs_wrapper_ptr, SkinsOfMeshes* in_skinsOfMeshes_ptr)
    :ComponentDataClass<SkinCompEntity, static_cast<componentID>(componentIDenum::Skin), "Skin", sparse_set>(in_ecs_wrapper_ptr),
     skinsOfMeshes_ptr(in_skinsOfMeshes_ptr)
{
}

SkinComp::~SkinComp()
{
}

void SkinComp::Update() //ComponentSparseBaseClass
{
    skinsOfMeshes_ptr->StartRecordingNodesMatrices();

    componentID nodeGlobalMatrix_componentID = static_cast<componentID>(componentIDenum::LateNodeGlobalMatrix);
    LateNodeGlobalMatrixComp* const nodeGlobalMatrixComp_ptr = static_cast<LateNodeGlobalMatrixComp*>(ecsWrapper_ptr->GetComponentByID(nodeGlobalMatrix_componentID));

    for (auto& this_comp_entity: componentEntities)
        this_comp_entity.Update(nodeGlobalMatrixComp_ptr, skinsOfMeshes_ptr);
}
