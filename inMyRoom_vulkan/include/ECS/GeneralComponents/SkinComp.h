#pragma once

#include "ECS/GeneralCompEntities/SkinCompEntity.h"
#include "ECS/ComponentSparseBaseClass.h"

#include "ECS/ComponentsIDsEnum.h"

#include "Meshes/SkinsOfMeshes.h"

class SkinComp 
    :public ComponentSparseBaseClass<SkinCompEntity>
{
public:
    SkinComp(ECSwrapper* const in_ecs_wrapper_ptr, SkinsOfMeshes* in_skinsOfMeshes_ptr);
    ~SkinComp();

    std::vector<std::pair<std::string, MapType>> GetComponentInitMapFields() override;

    void Update() override;
    void FixedUpdate() override {};
    void AsyncInput(InputType input_type, void* struct_data = nullptr) override {};

private:
    SkinsOfMeshes* skinsOfMeshes_ptr;
};