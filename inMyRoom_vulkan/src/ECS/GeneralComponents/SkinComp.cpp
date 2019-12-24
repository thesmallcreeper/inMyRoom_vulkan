#include "ECS/GeneralComponents/SkinComp.h"
#include "ECS/GeneralComponents/NodeGlobalMatrixComp.h"

#include "ECS/ECSwrapper.h"

SkinComp::SkinComp(ECSwrapper* const in_ecs_wrapper_ptr, SkinsOfMeshes* in_skinsOfMeshes_ptr)
    :ComponentSparseBaseClass<SkinCompEntity>(static_cast<componentID>(componentIDenum::Skin), "Skin", in_ecs_wrapper_ptr),
     skinsOfMeshes_ptr(in_skinsOfMeshes_ptr)
{
    SkinCompEntity::skinComp_ptr = this;
}

SkinComp::~SkinComp()
{
    SkinCompEntity::skinComp_ptr = nullptr;
}

std::vector<std::pair<std::string, MapType>> SkinComp::GetComponentInitMapFields()
{
    return SkinCompEntity::GetComponentInitMapFields();
}

void SkinComp::Update() //ComponentSparseBaseClass
{
    skinsOfMeshes_ptr->StartRecordingNodesMatrixes();

    componentID nodeGlobalMatrix_componentID = static_cast<componentID>(componentIDenum::NodeGlobalMatrix);
    NodeGlobalMatrixComp* const nodeGlobalMatrixComp_ptr = static_cast<NodeGlobalMatrixComp*>(ecsWrapper_ptr->GetComponentByID(nodeGlobalMatrix_componentID));

    for (size_t index = 0; index < componentEntitiesSparse.size(); index++)
        componentEntitiesSparse[index].Update(nodeGlobalMatrixComp_ptr, skinsOfMeshes_ptr);
}

