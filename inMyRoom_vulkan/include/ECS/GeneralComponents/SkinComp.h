#pragma once

#include "ECS/GeneralCompEntities/SkinCompEntity.h"
#include "ECS/ComponentDataClass.h"

#include "ECS/ComponentsIDsEnum.h"

#include "Graphics/Meshes/SkinsOfMeshes.h"

class SkinComp final
    : public ComponentDataClass<SkinCompEntity, static_cast<componentID>(componentIDenum::Skin), "Skin", sparse_set>
{
public:
    explicit SkinComp(ECSwrapper* const in_ecs_wrapper_ptr, SkinsOfMeshes* in_skinsOfMeshes_ptr);

private:
    SkinsOfMeshes* skinsOfMeshes_ptr;
};