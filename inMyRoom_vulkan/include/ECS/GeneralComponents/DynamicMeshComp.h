#pragma once

#include "ECS/GeneralCompEntities/DynamicMeshCompEntity.h"
#include "ECS/ComponentDataClass.h"

#include "ECS/ComponentsIDsEnum.h"

#include "Graphics/Meshes/SkinsOfMeshes.h"
#include "Graphics/DynamicMeshes.h"

class DynamicMeshComp final
    : public ComponentDataClass<DynamicMeshCompEntity, static_cast<componentID>(componentIDenum::DynamicMesh), "DynamicMesh", sparse_set>
{
public:
    DynamicMeshComp(ECSwrapper* ecs_wrapper_ptr, DynamicMeshes* dynamicMeshes_ptr, MeshesOfNodes* meshesOfNodes_ptr);

    void Update() override;
    void ToBeRemovedCallback(const std::vector<std::pair<Entity, Entity>>& callback_ranges) override;

private:
    DynamicMeshes* dynamicMeshes_ptr;
    MeshesOfNodes* meshesOfNodes_ptr;
};