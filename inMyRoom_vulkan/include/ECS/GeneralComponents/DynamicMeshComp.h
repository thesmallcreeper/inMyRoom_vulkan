#pragma once

#include "ECS/GeneralCompEntities/DynamicMeshCompEntity.h"
#include "ECS/ComponentDataClass.h"

#include "ECS/ComponentsIDsEnum.h"

#include "Graphics/Meshes/SkinsOfMeshes.h"

class DynamicMeshComp final
    : public ComponentDataClass<DynamicMeshCompEntity, static_cast<componentID>(componentIDenum::DynamicMesh), "DynamicMesh", sparse_set>
{
public:
    explicit DynamicMeshComp(ECSwrapper* const in_ecs_wrapper_ptr, SkinsOfMeshes* in_skinsOfMeshes_ptr);

private:
    SkinsOfMeshes* skinsOfMeshes_ptr;
};