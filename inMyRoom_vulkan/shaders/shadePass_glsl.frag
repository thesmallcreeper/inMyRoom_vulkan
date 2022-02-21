#version 460

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_shader_16bit_storage : require
#extension GL_EXT_shader_8bit_storage : require
#extension GL_EXT_ray_query : require

#include "common/structs/ModelMatrices.h"
#include "common/structs/MaterialParameters.h"
#include "common/structs/PrimitiveInstanceParameters.h"
#include "common/intersectOriginTriangle.glsl"
#include "common/rng.glsl"

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
layout (push_constant) uniform PushConstants
{
    layout(offset = 0)  uvec2 viewportSize;
    layout(offset = 8)  uint frameIndex;
    layout(offset = 12) uint alpha_one;

};

void ConfirmNonOpaqueIntersection(rayQueryEXT query)
{
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

vec4 SampleTextureBarycentric(vec2 barycoords, vec2 barycoordsDx, vec2 barycoordsDy, vec2 uv_0, vec2 uv_1, vec2 uv_2, uint texture_index)
{
    vec2 uv_edge_1 = uv_1 - uv_0;
    vec2 uv_edge_2 = uv_2 - uv_0;
    vec2 uv = uv_0 + barycoords.x * uv_edge_1 + barycoords.y * uv_edge_2;

    vec2 uv_dx = barycoordsDx.x * uv_edge_1 + barycoordsDx.y * uv_edge_2;
    vec2 uv_dy = barycoordsDy.x * uv_edge_1 + barycoordsDy.y * uv_edge_2;

    vec4 text_color = textureGrad(textures[texture_index], uv, uv_dx, uv_dy);
    return text_color;
}

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
    mat4x4 norm_matrix = viewMatrix * model_matrices[matrixOffset].normalMatrix;

    // Intersect triangle
    uint pos_descriptorIndex = uint(primitivesInstancesParameters[frag_primitiveInstance].positionDescriptorIndex);
    uint pos_offset = primitivesInstancesParameters[frag_primitiveInstance].positionOffset;
    vec4 vert_0 = pos_matrix * vec4verticesBuffers[pos_descriptorIndex].data[pos_offset + p_0_index];
    vec4 vert_1 = pos_matrix * vec4verticesBuffers[pos_descriptorIndex].data[pos_offset + p_1_index];
    vec4 vert_2 = pos_matrix * vec4verticesBuffers[pos_descriptorIndex].data[pos_offset + p_2_index];

    vec3 edge_1 = vec3(vert_1 - vert_0);
    vec3 edge_2 = vec3(vert_2 - vert_0);

    IntersectOriginTriangleResult intersect_result = IntersectOriginTriangle(vec3(vert_0), edge_1, edge_2, ray_dir);

    vec3 face_normal_cameraspace = normalize(cross(edge_1, edge_2));
    float dot_face_ray = dot(face_normal_cameraspace, ray_dir);
    if (dot_face_ray > 0.f) {
        face_normal_cameraspace = - face_normal_cameraspace;
    }

    // Interpolate vertices
    vec3 vertices_normal;
    {
        uint normal_descriptorIndex = uint(primitivesInstancesParameters[frag_primitiveInstance].normalDescriptorIndex);
        uint normal_offset = primitivesInstancesParameters[frag_primitiveInstance].normalOffset;
        vec4 normal_0 = vec4verticesBuffers[normal_descriptorIndex].data[normal_offset + p_0_index];
        vec4 normal_1 = vec4verticesBuffers[normal_descriptorIndex].data[normal_offset + p_1_index];
        vec4 normal_2 = vec4verticesBuffers[normal_descriptorIndex].data[normal_offset + p_2_index];

        vec4 normal_edge_1 = normal_1 - normal_0;
        vec4 normal_edge_2 = normal_2 - normal_0;

        vec4 normal_interpolated = normal_0 + intersect_result.barycoords.x * normal_edge_1 + intersect_result.barycoords.y * normal_edge_2;
        vertices_normal = normalize(vec3(norm_matrix * normal_interpolated));

        if (dot_face_ray > 0.f) {
            vertices_normal = - vertices_normal;
        }
    }

    vec3 vertices_tangent;
    vec3 vertices_bitangent;
    {
        uint tangent_descriptorIndex = uint(primitivesInstancesParameters[frag_primitiveInstance].tangentDescriptorIndex);
        uint tangent_offset = primitivesInstancesParameters[frag_primitiveInstance].tangentOffset;
        vec4 tangent_0 = vec4verticesBuffers[tangent_descriptorIndex].data[tangent_offset + p_0_index];
        vec4 tangent_1 = vec4verticesBuffers[tangent_descriptorIndex].data[tangent_offset + p_1_index];
        vec4 tangent_2 = vec4verticesBuffers[tangent_descriptorIndex].data[tangent_offset + p_2_index];

        float orientation = tangent_0.w;

        vec3 tangent_edge_1 = vec3(tangent_1 - tangent_0);
        vec3 tangent_edge_2 = vec3(tangent_2 - tangent_0);

        vec3 tangent_interpolated = vec3(tangent_0) + intersect_result.barycoords.x * tangent_edge_1 + intersect_result.barycoords.y * tangent_edge_2;
        vec3 vertices_orientated = vec3(pos_matrix * vec4(tangent_interpolated, 0.f));

        vertices_tangent = normalize(vertices_orientated - dot(vertices_orientated, vertices_normal) * vertices_normal);
        vertices_bitangent = cross(vertices_normal, vertices_tangent) * orientation;
    }

    vec4 vertex_color;
    {
        uint color_descriptorIndex = uint(primitivesInstancesParameters[frag_primitiveInstance].colorDescriptorIndex);
        uint color_offset = primitivesInstancesParameters[frag_primitiveInstance].colorOffset;
        uint color_stepMult = uint(primitivesInstancesParameters[frag_primitiveInstance].colorStepMultiplier);
        vec4 color_0 = vec4verticesBuffers[color_descriptorIndex].data[color_offset + p_0_index * color_stepMult];
        vec4 color_1 = vec4verticesBuffers[color_descriptorIndex].data[color_offset + p_1_index * color_stepMult];
        vec4 color_2 = vec4verticesBuffers[color_descriptorIndex].data[color_offset + p_2_index * color_stepMult];

        vec4 color_edge_1 = color_1 - color_0;
        vec4 color_edge_2 = color_2 - color_0;

        vertex_color = color_0 + intersect_result.barycoords.x * color_edge_1 + intersect_result.barycoords.y * color_edge_2;
    }

    // Texture
    uint material_index = uint(primitivesInstancesParameters[frag_primitiveInstance].material);
    uint uv_descriptorIndex = uint(primitivesInstancesParameters[frag_primitiveInstance].texcoordsDescriptorIndex);
    uint uv_offset = primitivesInstancesParameters[frag_primitiveInstance].texcoordsOffset;
    uint uv_stepMult = uint(primitivesInstancesParameters[frag_primitiveInstance].texcoordsStepMultiplier);
    MaterialParameters this_materialParameters = materialsParameters[material_index];

    vec4 text_color;
    {
        vec2 uv_0 = vec2verticesBuffers[uv_descriptorIndex].data[uv_offset + p_0_index * uv_stepMult + this_materialParameters.baseColorTexCoord];
        vec2 uv_1 = vec2verticesBuffers[uv_descriptorIndex].data[uv_offset + p_1_index * uv_stepMult + this_materialParameters.baseColorTexCoord];
        vec2 uv_2 = vec2verticesBuffers[uv_descriptorIndex].data[uv_offset + p_2_index * uv_stepMult + this_materialParameters.baseColorTexCoord];

        vec4 sample_color = SampleTextureBarycentric(intersect_result.barycoords, intersect_result.barycoordsDx, intersect_result.barycoordsDy,
                                                     uv_0, uv_1, uv_2, uint(this_materialParameters.baseColorTexture));
        text_color = this_materialParameters.baseColorFactors * sample_color;
    }

    vec3 text_normal;
    {
        vec2 uv_0 = vec2verticesBuffers[uv_descriptorIndex].data[uv_offset + p_0_index * uv_stepMult + this_materialParameters.normalTexCoord];
        vec2 uv_1 = vec2verticesBuffers[uv_descriptorIndex].data[uv_offset + p_1_index * uv_stepMult + this_materialParameters.normalTexCoord];
        vec2 uv_2 = vec2verticesBuffers[uv_descriptorIndex].data[uv_offset + p_2_index * uv_stepMult + this_materialParameters.normalTexCoord];

        vec3 sample_normal = SampleTextureBarycentric(intersect_result.barycoords, intersect_result.barycoordsDx, intersect_result.barycoordsDy,
                                                      uv_0, uv_1, uv_2, uint(this_materialParameters.normalTexture)).xyz;

        sample_normal = sample_normal * 2.f - 1.f;
        sample_normal *= vec3(this_materialParameters.normalScale, this_materialParameters.normalScale, 1.f);
        text_normal = normalize(sample_normal);
    }

    vec2 roughness_metallic_pair;
    {
        vec2 uv_0 = vec2verticesBuffers[uv_descriptorIndex].data[uv_offset + p_0_index * uv_stepMult + this_materialParameters.metallicRoughnessTexCoord];
        vec2 uv_1 = vec2verticesBuffers[uv_descriptorIndex].data[uv_offset + p_1_index * uv_stepMult + this_materialParameters.metallicRoughnessTexCoord];
        vec2 uv_2 = vec2verticesBuffers[uv_descriptorIndex].data[uv_offset + p_2_index * uv_stepMult + this_materialParameters.metallicRoughnessTexCoord];

        roughness_metallic_pair = SampleTextureBarycentric(intersect_result.barycoords, intersect_result.barycoordsDx, intersect_result.barycoordsDy,
                                                           uv_0, uv_1, uv_2, uint(this_materialParameters.metallicRoughnessTexture)).xy;
    }

    // Evaluate
    vec4 color = text_color * this_materialParameters.baseColorFactors * vertex_color;
    vec3 normal = vertices_tangent * text_normal.x + vertices_bitangent * text_normal.y + vertices_normal * text_normal.z;
    float roughness = roughness_metallic_pair.x;
    float metallic = roughness_metallic_pair.y;

    // Ray trace the sky
    vec3 out_sum = vec3(0.f, 0.f, 0.f);

    vec3 face_normal = vec3(inverseViewMatrix * vec4(face_normal_cameraspace, 0.f));
    face_normal = normalize(face_normal);

    vec3 sun_color = vec3(1.f, 1.f, 1.f);
    vec3 light_dir = normalize(vec3(-0.1f, -0.8f, 0.4f));
    if (dot(light_dir, face_normal) > -0.0046456f) {
        vec3 origin_pos_camera = intersect_result.distance * vec3(ray_dir);
        vec4 origin_pos_world = inverseViewMatrix * vec4(origin_pos_camera, 1.f);

        uint rng_state = InitRNG(gl_FragCoord.xy, viewportSize, frameIndex);

        uint max_iterators = 16;
        for (uint i = 0; i != max_iterators; ++i) {

            rayQueryEXT query;
            rayQueryInitializeEXT(query, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, origin_pos_world.xyz, 0.001f, light_dir, 10000.f);
            while (rayQueryProceedEXT(query)) {
                if (rayQueryGetIntersectionTypeEXT(query, false) == gl_RayQueryCandidateIntersectionTriangleEXT) {
                    ConfirmNonOpaqueIntersection(query);
                }
            }

            if (rayQueryGetIntersectionTypeEXT(query, true) == gl_RayQueryCommittedIntersectionNoneEXT) {
                out_sum += sun_color;
            }
        }
        out_sum /= float(max_iterators);
    }
    out_sum += vec3(0.05f, 0.05f, 0.05f);

    // Color
    if (alpha_one != 0) {
        color_out = vec4(out_sum * color.xyz, 1.f);
    } else {
        color_out = vec4(out_sum * color.xyz, 0.f);
    }
}