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
#include "common/brdf.glsl"

#define DOT_ANGLE_SLACK 0.005f
#define MIN_ROUGHNESS 0.01f

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
    // TODO better mipmapping
    float inter_text_alpha = textureLod(textures[inter_base_color_texture_index], inter_uv, 0.f).a;

    if (inter_text_alpha > inter_materialParameters.alphaCutoff) {
        rayQueryConfirmIntersectionEXT(query);
    }
}

vec4 SampleTextureBarycentric(vec2 barycoords, vec2 barycoordsDx, vec2 barycoordsDy,
                              vec2 uv_0, vec2 uv_1, vec2 uv_2, uint texture_index)
{
    vec2 uv_edge_1 = uv_1 - uv_0;
    vec2 uv_edge_2 = uv_2 - uv_0;
    vec2 uv = uv_0 + barycoords.x * uv_edge_1 + barycoords.y * uv_edge_2;

    vec2 uv_dx = barycoordsDx.x * uv_edge_1 + barycoordsDx.y * uv_edge_2;
    vec2 uv_dy = barycoordsDy.x * uv_edge_1 + barycoordsDy.y * uv_edge_2;

    vec4 text_color = textureGrad(textures[texture_index], uv, uv_dx, uv_dy);
    return text_color;
}

struct BounceEvaluation {
    vec3 origin;
    vec3 originDx;
    vec3 originDy;

    vec3 dir;
    vec3 dirDx;
    vec3 dirDy;

    vec3 next_bounce_light_factor;
    vec3 light_return;
};

BounceEvaluation EvaluateBounce(uint primitive_instance, uint triangle_index,
                                vec3 ray_dir, vec3 dir_dx, vec3 dir_dy,
                                vec3 origin, vec3 origin_dx, vec3 origin_dy,
                                vec3 light_factor, inout uint rng_state)
{
    // Get indices
    uint indices_offset = primitivesInstancesParameters[primitive_instance].indicesOffset + uint(primitivesInstancesParameters[primitive_instance].indicesSetMultiplier) * triangle_index;
    uint p_0_index = uintVerticesBuffers[0].data[indices_offset];
    uint p_1_index = uintVerticesBuffers[0].data[indices_offset + 1];
    uint p_2_index = uintVerticesBuffers[0].data[indices_offset + 2];

    // Get view matrix
    uint matrixOffset = uint(primitivesInstancesParameters[primitive_instance].matricesOffset);
    mat4x4 pos_matrix = model_matrices[matrixOffset].positionMatrix;
    mat4x4 norm_matrix = model_matrices[matrixOffset].normalMatrix;

    // Intersect triangle
    uint pos_descriptorIndex = uint(primitivesInstancesParameters[primitive_instance].positionDescriptorIndex);
    uint pos_offset = primitivesInstancesParameters[primitive_instance].positionOffset;
    vec4 vert_0 = pos_matrix * vec4verticesBuffers[pos_descriptorIndex].data[pos_offset + p_0_index];
    vec4 vert_1 = pos_matrix * vec4verticesBuffers[pos_descriptorIndex].data[pos_offset + p_1_index];
    vec4 vert_2 = pos_matrix * vec4verticesBuffers[pos_descriptorIndex].data[pos_offset + p_2_index];

    vec3 edge_1 = vec3(vert_1 - vert_0);
    vec3 edge_2 = vec3(vert_2 - vert_0);

    IntersectTriangleResult intersect_result = IntersectTriangle(vec3(vert_0), edge_1, edge_2,
                                                                 ray_dir, dir_dx, dir_dy,
                                                                 origin, origin_dx, origin_dy);

    vec3 face_normal = normalize(cross(edge_1, edge_2));
    float dot_face_ray = dot(face_normal, ray_dir);
    if (dot_face_ray > 0.f) {
        face_normal = - face_normal;
    }

    // Interpolate vertices
    vec3 vertices_normal;
    {
        uint normal_descriptorIndex = uint(primitivesInstancesParameters[primitive_instance].normalDescriptorIndex);
        uint normal_offset = primitivesInstancesParameters[primitive_instance].normalOffset;
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
        uint tangent_descriptorIndex = uint(primitivesInstancesParameters[primitive_instance].tangentDescriptorIndex);
        uint tangent_offset = primitivesInstancesParameters[primitive_instance].tangentOffset;
        vec4 tangent_0 = vec4verticesBuffers[tangent_descriptorIndex].data[tangent_offset + p_0_index];
        vec4 tangent_1 = vec4verticesBuffers[tangent_descriptorIndex].data[tangent_offset + p_1_index];
        vec4 tangent_2 = vec4verticesBuffers[tangent_descriptorIndex].data[tangent_offset + p_2_index];

        float orientation = -tangent_0.w;

        vec3 tangent_edge_1 = vec3(tangent_1 - tangent_0);
        vec3 tangent_edge_2 = vec3(tangent_2 - tangent_0);

        vec3 tangent_interpolated = vec3(tangent_0) + intersect_result.barycoords.x * tangent_edge_1 + intersect_result.barycoords.y * tangent_edge_2;
        vec3 vertices_orientated = vec3(pos_matrix * vec4(tangent_interpolated, 0.f));

        vertices_tangent = normalize(vertices_orientated - dot(vertices_orientated, vertices_normal) * vertices_normal);
        vertices_bitangent = cross(vertices_normal, vertices_tangent) * orientation;
    }

    vec4 vertex_color;
    {
        uint color_descriptorIndex = uint(primitivesInstancesParameters[primitive_instance].colorDescriptorIndex);
        uint color_offset = primitivesInstancesParameters[primitive_instance].colorOffset;
        uint color_stepMult = uint(primitivesInstancesParameters[primitive_instance].colorStepMultiplier);
        vec4 color_0 = vec4verticesBuffers[color_descriptorIndex].data[color_offset + p_0_index * color_stepMult];
        vec4 color_1 = vec4verticesBuffers[color_descriptorIndex].data[color_offset + p_1_index * color_stepMult];
        vec4 color_2 = vec4verticesBuffers[color_descriptorIndex].data[color_offset + p_2_index * color_stepMult];

        vec4 color_edge_1 = color_1 - color_0;
        vec4 color_edge_2 = color_2 - color_0;

        vertex_color = color_0 + intersect_result.barycoords.x * color_edge_1 + intersect_result.barycoords.y * color_edge_2;
    }

    // Texture
    uint material_index = uint(primitivesInstancesParameters[primitive_instance].material);
    uint uv_descriptorIndex = uint(primitivesInstancesParameters[primitive_instance].texcoordsDescriptorIndex);
    uint uv_offset = primitivesInstancesParameters[primitive_instance].texcoordsOffset;
    uint uv_stepMult = uint(primitivesInstancesParameters[primitive_instance].texcoordsStepMultiplier);
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
    vec3 normal_tangent = normalize(vertices_tangent - dot(vertices_tangent, normal) * normal);
    vec3 normal_bitangent = cross(normal, normal_tangent);
    float roughness = roughness_metallic_pair.x;
    roughness = roughness + (1.f - roughness) * MIN_ROUGHNESS;
    float metallic = roughness_metallic_pair.y;

    vec3 viewVector = -ray_dir;
    vec3 origin_pos = origin + intersect_result.distance * vec3(ray_dir);

    // Ray trace the sun
    vec3 light_sum = vec3(0.f, 0.f, 0.f);

    vec3 sun_color = vec3(1.f, 0.95f, 0.95f) * 1.67e9f;
    vec3 light_dir_world = normalize(vec3(-0.1f, -0.8f, 0.4f));
    vec3 light_dir = vec3(viewMatrix * vec4(light_dir_world, 0.f));
    vec3 light_dir_tangent = vec3(viewMatrix * vec4(0.f, 0.f, 1.f, 0.f));
    light_dir_tangent = normalize(light_dir_tangent - dot(light_dir_tangent, light_dir) * light_dir);
    vec3 light_dir_bitangent = cross(light_dir, light_dir_tangent);
    float light_cone_halfarc_rads = 0.266f * 0.01745329252f;

    if (dot(light_dir, face_normal) > -sin(light_cone_halfarc_rads) &&
        dot(viewVector, normal) > DOT_ANGLE_SLACK)
    {
        uint sun_iterators = 1;
        for (uint i = 0; i != sun_iterators; ++i) {
            vec3 random_light_dir_zaxis = RandomDirInCone(cos(light_cone_halfarc_rads), rng_state);
            vec3 random_light_dir = light_dir_bitangent * random_light_dir_zaxis.x + light_dir_tangent * random_light_dir_zaxis.y + light_dir * random_light_dir_zaxis.z;
            float random_light_dir_PDF = RandomDirInConePDF(cos(light_cone_halfarc_rads));

            if (dot(face_normal, random_light_dir) > DOT_ANGLE_SLACK &&
                dot(random_light_dir, normal) > DOT_ANGLE_SLACK)
            {
                rayQueryEXT query;
                rayQueryInitializeEXT(query, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, origin_pos, 0.0001f, random_light_dir, 100000.f);
                while (rayQueryProceedEXT(query)) {
                    if (rayQueryGetIntersectionTypeEXT(query, false) == gl_RayQueryCandidateIntersectionTriangleEXT) {
                        ConfirmNonOpaqueIntersection(query);
                    }
                }

                if (rayQueryGetIntersectionTypeEXT(query, true) == gl_RayQueryCommittedIntersectionNoneEXT) {
                    light_sum += sun_color * ((BRDF(color.xyz, roughness, metallic, viewVector, random_light_dir, normal) * dot(normal, random_light_dir)) / random_light_dir_PDF);
                }
            }
        }
        light_sum *= light_factor / float(sun_iterators);
    }

    // Bounce!
    vec3 viewVector_normalspace = transpose(mat3(normal_tangent, normal_bitangent, normal)) * viewVector;

    float cosine_weighted_chance = (1.f - metallic) * 0.4f + 0.2f;

    float u_0 = RandomFloat(rng_state);
    vec3 ray_bounce_normalspace;
    vec3 bounce_halfvector_normalspace;
    if ( u_0 <= cosine_weighted_chance ) {
        ray_bounce_normalspace = RandomCosinWeightedHemi(rng_state);
        bounce_halfvector_normalspace = normalize(ray_bounce_normalspace + viewVector_normalspace);
    } else {
        bounce_halfvector_normalspace = RandomGXXhalfvector(roughness, rng_state);
        ray_bounce_normalspace = 2.f * dot(viewVector_normalspace, bounce_halfvector_normalspace) * bounce_halfvector_normalspace - viewVector_normalspace;
    }

    vec3 ray_bounce = normal_tangent * ray_bounce_normalspace.x + normal_bitangent * ray_bounce_normalspace.y + normal * ray_bounce_normalspace.z;
    vec3 bounce_halfvector = normal_tangent * bounce_halfvector_normalspace.x + normal_bitangent * bounce_halfvector_normalspace.y + normal * bounce_halfvector_normalspace.z;
    vec3 bounce_light_factor = vec3(0.f);
    if (dot(bounce_halfvector_normalspace, viewVector_normalspace) > DOT_ANGLE_SLACK &&
        dot(ray_bounce, face_normal) > DOT_ANGLE_SLACK &&
        dot(viewVector, normal) > DOT_ANGLE_SLACK &&
        dot(ray_bounce, normal) > DOT_ANGLE_SLACK)
    {
        float ray_bounce_PDF_cosin = RandomCosinWeightedHemiPDF(ray_bounce_normalspace.z);

        float halfvector_PDF_GGX = RandomGXXhalfvectorPDF(roughness, bounce_halfvector_normalspace.z);
        float ray_bounce_PDF_GGX = halfvector_PDF_GGX / (4.f * dot(bounce_halfvector_normalspace, viewVector_normalspace));

        float ray_bounce_PDF = ray_bounce_PDF_cosin * cosine_weighted_chance + ray_bounce_PDF_GGX * (1.f - cosine_weighted_chance);

        bounce_light_factor = light_factor * ((BRDF(color.xyz, roughness, metallic, viewVector, ray_bounce, normal) * dot(ray_bounce, normal)) / ray_bounce_PDF);
    }

    // Return
    BounceEvaluation return_bounce_evaluation;

    return_bounce_evaluation.origin = origin_pos;
    return_bounce_evaluation.originDx = intersect_result.barycoordsDx.x * edge_1 + intersect_result.barycoordsDx.y * edge_2;
    return_bounce_evaluation.originDy = intersect_result.barycoordsDy.x * edge_1 + intersect_result.barycoordsDy.y * edge_2;

    return_bounce_evaluation.dir = ray_bounce;
    return_bounce_evaluation.dirDx = dir_dx - 2.f * dot(dir_dx, bounce_halfvector) * bounce_halfvector;
    return_bounce_evaluation.dirDx = dir_dy - 2.f * dot(dir_dy, bounce_halfvector) * bounce_halfvector;

    return_bounce_evaluation.next_bounce_light_factor = bounce_light_factor;
    return_bounce_evaluation.light_return = light_sum;

    return return_bounce_evaluation;
}

void main()
{
    uint rng_state = InitRNG(gl_FragCoord.xy, viewportSize, frameIndex);

    // Read input attachment
    uvec2 frag_pair = uvec2(subpassLoad(visibilityInput));

    uint primitive_instance = frag_pair.x;
    uint triangle_index = frag_pair.y;

    vec3 origin = vec3(0.f);
    vec3 origin_dx = vec3(0.f);
    vec3 origin_dy = vec3(0.f);

    vec3 ray_dir = normalize(vert_normal.xyz);
    vec3 ray_dir_dx = dFdx(ray_dir);
    vec3 ray_dir_dy = dFdy(ray_dir);

    vec3 light_factor = vec3(1.f);
    vec3 color_sum = vec3(0.f);

    uint max_depth = 8;
    uint i = 0;
    while(true) {
        BounceEvaluation eval = EvaluateBounce(primitive_instance, triangle_index,
                                ray_dir, ray_dir_dx, ray_dir_dy,
                                origin, origin_dx, origin_dy,
                                light_factor, rng_state);

        origin = eval.origin;
        origin_dx = eval.originDx;
        origin_dy = eval.originDy;

        ray_dir = eval.dir;
        ray_dir_dx = eval.dirDx;
        ray_dir_dy = eval.dirDy;

        light_factor = eval.next_bounce_light_factor;
        color_sum += eval.light_return;

        ++i;
        if (i == max_depth || light_factor == vec3(0.f))
            break;

        rayQueryEXT query;
        rayQueryInitializeEXT(query, topLevelAS, 0, 0xFF, origin, 0.0001f, ray_dir, 100000.f);
        while (rayQueryProceedEXT(query)) {
            if (rayQueryGetIntersectionTypeEXT(query, false) == gl_RayQueryCandidateIntersectionTriangleEXT) {
                ConfirmNonOpaqueIntersection(query);
            }
        }

        if (rayQueryGetIntersectionTypeEXT(query, true) == gl_RayQueryCommittedIntersectionNoneEXT) {
            vec3 sky_luminance = vec3(0.95f, 1.f, 1.f) * 8.e3f;
            color_sum += light_factor * sky_luminance;
            break;
        } else {
            primitive_instance = rayQueryGetIntersectionInstanceCustomIndexEXT(query, true) + rayQueryGetIntersectionGeometryIndexEXT(query, true);
            triangle_index = rayQueryGetIntersectionPrimitiveIndexEXT(query, true);
        }
    }

    // Color out
    if (alpha_one != 0) {
        color_out = vec4(color_sum, 1.f);
    } else {
        color_out = vec4(color_sum, 0.f);
    }
}