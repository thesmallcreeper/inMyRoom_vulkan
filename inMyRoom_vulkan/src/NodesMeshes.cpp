#include "NodesMeshes.h"

#include "MeshesPrimitives.h"
#include "Geometry/Sphere.h"

NodesMeshes::NodesMeshes(tinygltf::Model& in_model, MeshesPrimitives* in_meshesPrimitives_ptr,
                         Anvil::BaseDevice* const in_device_ptr)
    :
    meshesPrimitives_ptr(in_meshesPrimitives_ptr),
    device_ptr(in_device_ptr)
{
    size_t primitives_so_far = 0;
    for (tinygltf::Mesh& this_mesh : in_model.meshes)
    {
        MeshRange this_mesh_range;

        std::vector<vec> points_of_sphere;

        for (tinygltf::Primitive this_primitive : this_mesh.primitives)
        {
            meshesPrimitives_ptr->AddPrimitive(in_model, this_primitive);
            size_t begin_index_byte = meshesPrimitives_ptr->primitivesInitInfos.rbegin()[0].positionBufferOffset;
            size_t end_index_byte = meshesPrimitives_ptr->localPositionBuffer.size();

            float* begin_index = reinterpret_cast<float*>(meshesPrimitives_ptr->localPositionBuffer.data() + begin_index_byte);
            float* end_index = reinterpret_cast<float*>(meshesPrimitives_ptr->localPositionBuffer.data() + end_index_byte);

            for (float* this_ptr = begin_index; this_ptr != end_index; this_ptr += 3)
                points_of_sphere.emplace_back(this_ptr[0], this_ptr[1], this_ptr[2]);
        }

        this_mesh_range.primitiveFirstOffset = primitives_so_far;
        this_mesh_range.primitiveRangeSize = this_mesh.primitives.size();
        this_mesh_range.boundSphere = Sphere::OptimalEnclosingSphere(points_of_sphere.data(), points_of_sphere.size());

        meshes.emplace_back(this_mesh_range);

        primitives_so_far += this_mesh.primitives.size();
    }

    meshesPrimitives_ptr->FlashBuffersToDevice();
}

NodesMeshes::~NodesMeshes()
{
}

