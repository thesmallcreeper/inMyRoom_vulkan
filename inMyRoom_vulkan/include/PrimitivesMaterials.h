#pragma once

#include <string>

#include "misc/descriptor_set_create_info.h"
#include "wrappers/descriptor_set.h"
#include "wrappers/descriptor_set_group.h"

#include "tiny_gltf.h"

#include "MaterialsTextures.h"
#include "PrimitivesShaders.h"

class PrimitivesMaterials
{
public:
    PrimitivesMaterials(tinygltf::Model& in_model, MaterialsTextures* in_materialsTextures,
                        Anvil::BaseDevice* const in_device_ptr);
    ~PrimitivesMaterials();

public:
    Anvil::DescriptorSetGroupUniquePtr texturesOfMaterialsDescriptorSetGroup_uptr;
    std::vector<ShadersSpecs> materialsShadersSpecs;

private:
    Anvil::BaseDevice* const device_ptr;

    MaterialsTextures* materialsTextures_ptr;

    std::vector<Anvil::BufferUniquePtr> materialsFactorsBuffer_ptrs;
};
