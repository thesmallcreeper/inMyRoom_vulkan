#include "ECS/GeneralComponents/ModelDrawComp.h"

#include "ECS/ECSwrapper.h"

ModelDrawComp::ModelDrawComp(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentDataClass<ModelDrawCompEntity, static_cast<componentID>(componentIDenum::ModelDraw), "ModelDraw", sparse_set>(in_ecs_wrapper_ptr)
{
}

ModelDrawComp::~ModelDrawComp()
{
}

void ModelDrawComp::AddDrawInfos(std::vector<glm::mat4>& matrices,
                                 std::vector<DrawInfo>& draw_infos)
{
    auto nodeGlobalMatrix_componentID = static_cast<componentID>(componentIDenum::LateNodeGlobalMatrix);
    auto nodeGlobalMatrixComp_ptr = static_cast<const LateNodeGlobalMatrixComp*>(ecsWrapper_ptr->GetComponentByID(nodeGlobalMatrix_componentID));

    auto skin_componentID = static_cast<componentID>(componentIDenum::Skin);
    auto skinComp_ptr = static_cast<const SkinComp*>(ecsWrapper_ptr->GetComponentByID(skin_componentID));

    size_t containers_count = GetContainersCount();
    for(size_t i = 0; i != containers_count; ++i)
    {
        auto& this_container = GetContainerByIndex(containersUpdated);
        for(auto& this_comp_entity: this_container)
            this_comp_entity.AddDrawInfo(nodeGlobalMatrixComp_ptr,
                                         skinComp_ptr,
                                         matrices,
                                         draw_infos);
    }
}

