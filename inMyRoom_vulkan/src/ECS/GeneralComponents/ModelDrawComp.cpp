#include "ECS/GeneralComponents/ModelDrawComp.h"

#include "ECS/ECSwrapper.h"

ModelDrawComp::ModelDrawComp(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentDataClass<ModelDrawCompEntity, static_cast<componentID>(componentIDenum::ModelDraw), "ModelDraw", sparse_set>(in_ecs_wrapper_ptr)
{
}

ModelDrawComp::~ModelDrawComp()
{
}

void ModelDrawComp::AddDrawInfos(std::vector<ModelMatrices>& matrices,
                                 std::vector<DrawInfo>& draw_infos)
{
    auto nodeGlobalMatrix_componentID = static_cast<componentID>(componentIDenum::LateNodeGlobalMatrix);
    auto nodeGlobalMatrixComp_ptr = static_cast<const LateNodeGlobalMatrixComp*>(ecsWrapper_ptr->GetComponentByID(nodeGlobalMatrix_componentID));

    auto skin_componentID = static_cast<componentID>(componentIDenum::DynamicMesh);
    auto skinComp_ptr = static_cast<const DynamicMeshComp*>(ecsWrapper_ptr->GetComponentByID(skin_componentID));

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

void ModelDrawComp::ToBeRemovedCallback(const std::vector<std::pair<Entity, Entity>> &callback_ranges)
{
    size_t containers_count_when_start = GetContainersCount();
    for (const std::pair<Entity, Entity>& this_range : callback_ranges) {
        size_t container_index = this->entitiesHandler_ptr->GetContainerIndexOfEntity(this_range.first);
        auto& container = GetContainerByIndex(container_index);

        auto iterators = container.get_range_iterators(this_range);
        for (auto it = iterators.first; it != iterators.second; ++it) {
            it->ToBeRemovedCallback();
        }
    }
}

