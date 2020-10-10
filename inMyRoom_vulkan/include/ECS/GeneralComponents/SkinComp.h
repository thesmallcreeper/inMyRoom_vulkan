#pragma once

#include "ECS/GeneralCompEntities/SkinCompEntity.h"
#include "ECS/ComponentSparseBaseClass.h"

#include "ECS/ComponentsIDsEnum.h"

#include "Graphics/Meshes/SkinsOfMeshes.h"

class SkinComp 
    :public ComponentSparseBaseClass<SkinCompEntity, static_cast<componentID>(componentIDenum::Skin), "Skin">
{
public:
    SkinComp(ECSwrapper* const in_ecs_wrapper_ptr, SkinsOfMeshes* in_skinsOfMeshes_ptr);
    ~SkinComp();

    void Update() override;
    void AsyncInput(InputType input_type, void* struct_data = nullptr) override {};

    void CollisionCallback(Entity this_entity, const CollisionCallbackData& this_collisionCallbackData) override {};
private:
    SkinsOfMeshes* skinsOfMeshes_ptr;
};