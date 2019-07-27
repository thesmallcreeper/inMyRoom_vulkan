#pragma once

#include <string>

#include "misc/memory_allocator.h"
#include "misc/buffer_create_info.h"
#include "misc/descriptor_set_create_info.h"
#include "wrappers/descriptor_set.h"
#include "wrappers/descriptor_set_group.h"
#include "wrappers/buffer.h"

#include "tiny_gltf.h"

#include "TexturesOfMaterials.h"
#include "ShadersOfPrimitives.h"

class MaterialsOfPrimitives
{
public:
    MaterialsOfPrimitives(tinygltf::Model& in_model, TexturesOfMaterials* in_texturesOfMaterials_ptr,
                        Anvil::BaseDevice* const in_device_ptr);
    ~MaterialsOfPrimitives();

public:public:
    Anvil::DescriptorSetGroupUniquePtr materialsDescriptorSetGroup_uptr;
    std::vector<ShadersSpecs> materialsShadersSpecs;

private:
    Anvil::BufferUniquePtr CreateDeviceBufferForLocalBuffer(const std::vector<unsigned char>& in_localBuffer, Anvil::BufferUsageFlagBits in_bufferusageflag) const;
private:
    Anvil::BufferUniquePtr materialsFactorsBuffer_uptr;
    std::vector<unsigned char> localMaterialsFactorsBuffer;

    TexturesOfMaterials* texturesOfMaterials_ptr;

    Anvil::BaseDevice* const device_ptr;
};
