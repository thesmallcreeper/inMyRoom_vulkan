#include "ECS/GeneralComponents/SkinComp.h"
#include "ECS/GeneralComponents/LateNodeGlobalMatrixComp.h"

#include "ECS/ECSwrapper.h"

SkinComp::SkinComp(ECSwrapper* const in_ecs_wrapper_ptr, SkinsOfMeshes* in_skinsOfMeshes_ptr)
    :ComponentSparseBaseClass<SkinCompEntity, static_cast<componentID>(componentIDenum::Skin), "Skin">(in_ecs_wrapper_ptr),
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

    for (size_t index = 0; index < componentEntitiesSparse.size(); index++)
        componentEntitiesSparse[index].Update(nodeGlobalMatrixComp_ptr, skinsOfMeshes_ptr);
}
