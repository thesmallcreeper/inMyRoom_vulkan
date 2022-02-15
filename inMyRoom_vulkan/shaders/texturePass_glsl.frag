#version 460

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_shader_16bit_storage : require
#extension GL_EXT_shader_8bit_storage : require
#extension GL_EXT_ray_query : require

#include "common/structs/ModelMatrices.h"
#include "common/structs/MaterialParameters.h"
#include "common/structs/PrimitiveInstanceParameters.h"
#include "common/intersectOriginTriangle.glsl"

//
// In
layout( location = 0 ) in vec4 vert_normal;

//
// Out
layout( location = 0 ) out vec4 color_out;

//
// Descriptors

/// 0, 0
layout( set = 0 , binding = 0 ) uniform projectionMatrixBuffer
{
    mat4 viewMatrix;
    mat4 inverseViewMatrix;
    mat4 projectionMatrix;
};

/// 1, 0
layout( std430, set = 1 , binding = 0 ) readonly buffer worldSpaceMatricesBufferDescriptor
{
    ModelMatrices model_matrices[MATRICES_COUNT];
};

/// 2, 0
layout( std430, set = 2, binding = 0 ) readonly buffer uintVerticesBuffersDescriptors
{
    uint data[];
} uintVerticesBuffers [];

layout( std430, set = 2, binding = 0 ) readonly buffer vec2verticesBuffersDescriptors
{
    vec2 data[];
} vec2verticesBuffers [];

layout( std430, set = 2, binding = 0 ) readonly buffer vec4verticesBuffersDescriptors
{
    vec4 data[];
} vec4verticesBuffers [];

/// 3, 0
layout (std430, set = 3, binding = 0) readonly buffer materialsParametersBufferDescriptor
{
    MaterialParameters materialsParameters[MATERIALS_PARAMETERS_COUNT];
};

/// 3, 1
layout (set = 3, binding = 1) uniform sampler2D textures[TEXTURES_COUNT];

/// 4, 0
layout (set = 4, binding = 0) readonly buffer primitivesInstancesBufferDescriptor
{
    PrimitiveInstanceParameters primitivesInstancesParameters[INSTANCES_COUNT];
};

/// 4, 1
layout (input_attachment_index = 0, set = 4, binding = 1) uniform usubpassInput visibilityInput;

/// 4, 2
layout(set = 4, binding = 2) uniform accelerationStructureEXT topLevelAS;

//
// Push constants

void main()
{
    vec3 ray_dir = normalize(vert_normal.xyz);

    // Read input attachment
    uvec2 frag_pair = uvec2(subpassLoad(visibilityInput));
    uint frag_primitiveInstance = frag_pair.x;
    uint frag_triangleIndex = frag_pair.y;

    // Get indices
    uint indices_offset = primitivesInstancesParameters[frag_primitiveInstance].indicesOffset + uint(primitivesInstancesParameters[frag_primitiveInstance].indicesSetMultiplier) * frag_triangleIndex;
    uint p_0_index = uintVerticesBuffers[0].data[indices_offset];
    uint p_1_index = uintVerticesBuffers[0].data[indices_offset + 1];
    uint p_2_index = uintVerticesBuffers[0].data[indices_offset + 2];

    // Get view matrix
    uint matrixOffset = uint(primitivesInstancesParameters[frag_primitiveInstance].matricesOffset);
    mat4x4 pos_matrix = viewMatrix * model_matrices[matrixOffset].positionMatrix;

    // Intersect triangle
    uint pos_descriptorIndex = uint(primitivesInstancesParameters[frag_primitiveInstance].positionDescriptorIndex);
    uint pos_offset = primitivesInstancesParameters[frag_primitiveInstance].positionOffset;
    vec4 vert_0 = pos_matrix * vec4verticesBuffers[pos_descriptorIndex].data[pos_offset + p_0_index];
    vec4 vert_1 = pos_matrix * vec4verticesBuffers[pos_descriptorIndex].data[pos_offset + p_1_index];
    vec4 vert_2 = pos_matrix * vec4verticesBuffers[pos_descriptorIndex].data[pos_offset + p_2_index];

    vec3 edge_1 = vec3(vert_1 - vert_0);
    vec3 edge_2 = vec3(vert_2 - vert_0);

    IntersectOriginTriangleResult intersect_result = IntersectOriginTriangle(vec3(vert_0), edge_1, edge_2, ray_dir);

    // Texture
    uint material_index = uint(primitivesInstancesParameters[frag_primitiveInstance].material);
    MaterialParameters this_materialParameters = materialsParameters[material_index];

    uint uv_descriptorIndex = uint(primitivesInstancesParameters[frag_primitiveInstance].texcoordsDescriptorIndex);
    uint uv_offset = primitivesInstancesParameters[frag_primitiveInstance].texcoordsOffset;
    uint uv_stepMult = uint(primitivesInstancesParameters[frag_primitiveInstance].texcoordsStepMultiplier);
    vec2 uv_0 = vec2verticesBuffers[uv_descriptorIndex].data[uv_offset + p_0_index * uv_stepMult + this_materialParameters.baseColorTexCoord];
    vec2 uv_1 = vec2verticesBuffers[uv_descriptorIndex].data[uv_offset + p_1_index * uv_stepMult + this_materialParameters.baseColorTexCoord];
    vec2 uv_2 = vec2verticesBuffers[uv_descriptorIndex].data[uv_offset + p_2_index * uv_stepMult + this_materialParameters.baseColorTexCoord];

    vec2 uv_edge_1 = uv_1 - uv_0;
    vec2 uv_edge_2 = uv_2 - uv_0;
    vec2 uv = uv_0 + intersect_result.barycoords.x * uv_edge_1 + intersect_result.barycoords.y * uv_edge_2;

    vec2 uv_dx = intersect_result.barycoordsDx.x * uv_edge_1 + intersect_result.barycoordsDx.y * uv_edge_2;
    vec2 uv_dy = intersect_result.barycoordsDy.x * uv_edge_1 + intersect_result.barycoordsDy.y * uv_edge_2;

    uint base_color_texture_index = this_materialParameters.baseColorTexture;
    vec4 text_color = textureGrad(textures[base_color_texture_index], uv, uv_dx, uv_dy);

    // Ray trace the sky
    vec4 color_factor = vec4(1.f, 1.f, 1.f, 1.f);

    vec3 face_normal_cameraspace = cross(edge_1, edge_2);
    if (dot(face_normal_cameraspace, ray_dir) > 0.f) {
        face_normal_cameraspace = - face_normal_cameraspace;
    }
    vec3 face_normal = vec3(inverseViewMatrix * vec4(face_normal_cameraspace, 0.f));
    face_normal = normalize(face_normal);

    vec3 light_dir = vec3(-0.1f, -0.8f, 0.4f);
    if (dot(light_dir, face_normal) > 0.001f) {

        vec3 camera_pos = intersect_result.distance * vec3(ray_dir);
        vec4 camera_pos_vec4 = vec4(camera_pos, 1.f);
        vec4 world_space_pos = inverseViewMatrix * camera_pos_vec4;

        rayQueryEXT query;
        rayQueryInitializeEXT(query, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, world_space_pos.xyz, 0.001f, light_dir, 10000.f);
        while (rayQueryProceedEXT(query)) {
            if (rayQueryGetIntersectionTypeEXT(query, false) == gl_RayQueryCandidateIntersectionTriangleEXT) {
                uint intersect_primitiveInstance = rayQueryGetIntersectionInstanceCustomIndexEXT(query, false) + rayQueryGetIntersectionGeometryIndexEXT(query, false);
                uint intersect_triangleIndex = rayQueryGetIntersectionPrimitiveIndexEXT(query, false);

                uint inter_indices_offset = primitivesInstancesParameters[intersect_primitiveInstance].indicesOffset + 3 * intersect_triangleIndex;
                uint inter_p_0_index = uintVerticesBuffers[0].data[inter_indices_offset];
                uint inter_p_1_index = uintVerticesBuffers[0].data[inter_indices_offset + 1];
                uint inter_p_2_index = uintVerticesBuffers[0].data[inter_indices_offset + 2];

                uint inter_material_index = uint(primitivesInstancesParameters[intersect_primitiveInstance].material);
                MaterialParameters inter_materialParameters = materialsParameters[inter_material_index];

                uint inter_uv_descriptorIndex = uint(primitivesInstancesParameters[intersect_primitiveInstance].texcoordsDescriptorIndex);
                uint inter_uv_offset = primitivesInstancesParameters[intersect_primitiveInstance].texcoordsOffset;
                uint inter_uv_stepMult = uint(primitivesInstancesParameters[intersect_primitiveInstance].texcoordsStepMultiplier);
                vec2 inter_uv_0 = vec2verticesBuffers[inter_uv_descriptorIndex].data[inter_uv_offset + inter_p_0_index * inter_uv_stepMult + inter_materialParameters.baseColorTexCoord];
                vec2 inter_uv_1 = vec2verticesBuffers[inter_uv_descriptorIndex].data[inter_uv_offset + inter_p_1_index * inter_uv_stepMult + inter_materialParameters.baseColorTexCoord];
                vec2 inter_uv_2 = vec2verticesBuffers[inter_uv_descriptorIndex].data[inter_uv_offset + inter_p_2_index * inter_uv_stepMult + inter_materialParameters.baseColorTexCoord];

                vec2 inter_barycentric = rayQueryGetIntersectionBarycentricsEXT(query, false);
                vec2 inter_uv_edge_1 = inter_uv_1 - inter_uv_0;
                vec2 inter_uv_edge_2 = inter_uv_2 - inter_uv_0;
                vec2 inter_uv = inter_uv_0 + inter_barycentric.x * inter_uv_edge_1 + inter_barycentric.y * inter_uv_edge_2;

                uint inter_base_color_texture_index = inter_materialParameters.baseColorTexture;
                float inter_text_alpha = textureLod(textures[inter_base_color_texture_index], inter_uv, 0.f).a;

                if (inter_text_alpha > inter_materialParameters.alphaCutoff) {
                    rayQueryConfirmIntersectionEXT(query);
                }
            }
        }

        if (rayQueryGetIntersectionTypeEXT(query, true) != gl_RayQueryCommittedIntersectionNoneEXT) {
            color_factor = vec4(0.5f, 0.5f, 0.5f, 1.f);
        }
    } else {
        color_factor = vec4(0.5f, 0.5f, 0.5f, 1.f);
    }

    // Color
    color_out = color_factor * text_color * this_materialParameters.baseColorFactors;
}