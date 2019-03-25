#pragma once

#include <string>

#include "tiny_gltf.h"

#include "misc/descriptor_set_create_info.h"
#include "wrappers/descriptor_set.h"
#include "wrappers/descriptor_set_group.h"

#include "MaterialsTextures.h"
#include "PrimitivesShaders.h"

class PrimitivesMaterials
{
public:
    PrimitivesMaterials(tinygltf::Model& in_model, MaterialsTextures* in_materialsTextures, Anvil::BaseDevice* in_device_ptr);
    ~PrimitivesMaterials();

    Anvil::DescriptorSetGroupUniquePtr	dsg_ptr;
    std::vector<ShadersSpecs> materialsShadersSpecs;

private:
    std::vector<Anvil::BufferUniquePtr> materialsFactorsBuffer_ptrs;

    MaterialsTextures* materialsTextures_ptr;

    Anvil::BaseDevice* device_ptr;
};