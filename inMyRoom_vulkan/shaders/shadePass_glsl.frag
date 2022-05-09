#version 460

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_shader_16bit_storage : require
#extension GL_EXT_shader_8bit_storage : require
#extension GL_EXT_ray_query : require

#include "common/defines.h"
#include "common/structs/ModelMatrices.h"
#include "common/structs/MaterialParameters.h"
#include "common/structs/PrimitiveInstanceParameters.h"
#include "common/structs/LightParameters.h"
#include "common/intersectTriangle.glsl"
#include "common/rayOffsetFace.glsl"
#include "common/RoughnessLengthMap.h"
#include "common/rng.glsl"
#include "common/brdf.glsl"
#include "common/reservoir.glsl"
#include "common/samplesPosition.glsl"

#define MAX_DEPTH 3
#define DOT_ANGLE_SLACK 0.0087265f      // cos(89.5 degs)
#define MIN_ROUGHNESS 0.02f
#define USE_INCREASING_MOLLIFICATION
#define LIGHT_THRESHOLD 0.5e6f

// #define DEBUG_MIS

#define INF_DIST 100000.f

//
// In
layout( location = 0 ) in vec3 vert_normal;

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

/// 5, 0
#ifdef MULTISAMPLED_INPUT
layout (input_attachment_index = 0, set = 5, binding = 0) uniform usubpassInputMS visibilityInput;
#else
layout (input_attachment_index = 0, set = 5, binding = 0) uniform usubpassInput visibilityInput;
#endif

/// 6, 0
layout(set = 6, binding = 0) uniform accelerationStructureEXT topLevelAS;

/// 7, 0
layout (set = 7, binding = 0) readonly buffer lightsParametersBufferDescriptor
{
    LightParameters lightsParameters[MAX_LIGHTS_COUNT];
};

layout (set = 7, binding = 1) readonly buffer lightsCombinationsBufferDescriptor
{
    uint16_t lightsCombinations[MAX_COMBINATIONS_SIZE];
};

//
// Push constants
layout (push_constant) uniform PushConstants
{
    layout(offset = 0)  vec3 sky_luminance;
    layout(offset = 16) uvec2 viewportSize;
    layout(offset = 24) uint frameIndex;
    layout(offset = 28) uint lightConesIndices_offset;
    layout(offset = 32) uint lightConesIndices_size;
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

vec4 SampleTextureBarycentric(vec2 barycoords, BarycoordsDiffs bary_diffs,
                              vec2 uv_0, vec2 uv_1, vec2 uv_2, uint texture_index)
{
    vec2 uv_edge_1 = uv_1 - uv_0;
    vec2 uv_edge_2 = uv_2 - uv_0;
    vec2 uv = uv_0 + barycoords.x * uv_edge_1 + barycoords.y * uv_edge_2;

    vec2 uv_dx = bary_diffs.barycoordsDx.x * uv_edge_1 + bary_diffs.barycoordsDx.y * uv_edge_2;
    vec2 uv_dy = bary_diffs.barycoordsDy.x * uv_edge_1 + bary_diffs.barycoordsDy.y * uv_edge_2;

    vec4 text_color = textureGrad(textures[texture_index], uv, uv_dx, uv_dy);
    return text_color;
}

float WeightLightBalance(float pdf, float other_pdf) {
    float pdf_weight = pdf * pdf;
    float other_pdf_weight = other_pdf * other_pdf;
    return pdf_weight / (pdf_weight + other_pdf_weight);
}

struct BounceEvaluation {
    vec3 origin;
    RayDiffsOrigin originDiffs;

    vec3 dir;
    RayDiffsDir dirDiffs;

    vec3 next_bounce_light_factor;
    vec3 light_return;

    uint light_target;
    vec3 light_target_contribution;
};

BounceEvaluation EvaluateBounce(IntersectTriangleResult intersect_result,
                                uint primitive_instance, uint triangle_index,
                                vec3 origin, RayDiffsOrigin ray_originDiffs,
                                vec3 ray_dir, RayDiffsDir ray_dirDiffs,
                                vec3 light_factor, uint ray_depth,
                                inout float min_roughness,
                                inout uint rng_state)
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
    vec3 pos_0 = vec3(pos_matrix * vec4verticesBuffers[pos_descriptorIndex].data[pos_offset + p_0_index]);
    vec3 pos_1 = vec3(pos_matrix * vec4verticesBuffers[pos_descriptorIndex].data[pos_offset + p_1_index]);
    vec3 pos_2 = vec3(pos_matrix * vec4verticesBuffers[pos_descriptorIndex].data[pos_offset + p_2_index]);

    vec3 edge_1 = pos_1 - pos_0;
    vec3 edge_2 = pos_2 - pos_0;

    if (ray_depth == 0) {
        intersect_result = IntersectTriangle(pos_0, edge_1, edge_2,
                                             ray_dir, origin);
    }

    vec3 face_normal = normalize(cross(edge_1, edge_2));
    float dot_face_ray = dot(face_normal, ray_dir);
    if (dot_face_ray > 0.f) {
        face_normal = - face_normal;
    }

    // Interpolate vertices
    vec3 vertex_normal;
    {
        uint normal_descriptorIndex = uint(primitivesInstancesParameters[primitive_instance].normalDescriptorIndex);
        uint normal_offset = primitivesInstancesParameters[primitive_instance].normalOffset;
        vec4 normal_0 = vec4verticesBuffers[normal_descriptorIndex].data[normal_offset + p_0_index];
        vec4 normal_1 = vec4verticesBuffers[normal_descriptorIndex].data[normal_offset + p_1_index];
        vec4 normal_2 = vec4verticesBuffers[normal_descriptorIndex].data[normal_offset + p_2_index];

        vec4 normal_edge_1 = normal_1 - normal_0;
        vec4 normal_edge_2 = normal_2 - normal_0;

        vec4 normal_interpolated = normal_0 + intersect_result.barycoords.x * normal_edge_1 + intersect_result.barycoords.y * normal_edge_2;
        vertex_normal = normalize(vec3(norm_matrix * normal_interpolated));

        if (dot_face_ray > 0.f) {
            vertex_normal = - vertex_normal;
        }
    }

    vec3 vertex_tangent;
    vec3 vertex_bitangent;
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

        vertex_tangent = normalize(vertices_orientated - dot(vertices_orientated, vertex_normal) * vertex_normal);
        vertex_bitangent = cross(vertex_normal, vertex_tangent) * orientation;
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

    // Diffs propagation
    RayDiffsOrigin origin_rayDiffs = PropagateRayDiffsOrigin(ray_originDiffs, ray_dirDiffs, vertex_normal, ray_dir, intersect_result.distance);
    BarycoordsDiffs barycoords_rayDiffs = ComputeBaryDiffs(origin_rayDiffs, edge_1, edge_2, face_normal);

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

        vec4 sample_color = SampleTextureBarycentric(intersect_result.barycoords, barycoords_rayDiffs,
                                                     uv_0, uv_1, uv_2, uint(this_materialParameters.baseColorTexture));
        text_color = this_materialParameters.baseColorFactors * sample_color;
    }

    vec3 text_normal;
    float text_normal_length;
    {
        vec2 uv_0 = vec2verticesBuffers[uv_descriptorIndex].data[uv_offset + p_0_index * uv_stepMult + this_materialParameters.normalTexCoord];
        vec2 uv_1 = vec2verticesBuffers[uv_descriptorIndex].data[uv_offset + p_1_index * uv_stepMult + this_materialParameters.normalTexCoord];
        vec2 uv_2 = vec2verticesBuffers[uv_descriptorIndex].data[uv_offset + p_2_index * uv_stepMult + this_materialParameters.normalTexCoord];

        vec3 sample_normal = SampleTextureBarycentric(intersect_result.barycoords, barycoords_rayDiffs,
                                                      uv_0, uv_1, uv_2, uint(this_materialParameters.normalTexture)).xyz;

        sample_normal = sample_normal * 2.f - 1.f;
        sample_normal *= vec3(this_materialParameters.normalScale, this_materialParameters.normalScale, 1.f);
        text_normal_length = length(sample_normal);
        text_normal = sample_normal / text_normal_length;
    }

    vec2 roughness_metallic_pair;
    {
        vec2 uv_0 = vec2verticesBuffers[uv_descriptorIndex].data[uv_offset + p_0_index * uv_stepMult + this_materialParameters.metallicRoughnessTexCoord];
        vec2 uv_1 = vec2verticesBuffers[uv_descriptorIndex].data[uv_offset + p_1_index * uv_stepMult + this_materialParameters.metallicRoughnessTexCoord];
        vec2 uv_2 = vec2verticesBuffers[uv_descriptorIndex].data[uv_offset + p_2_index * uv_stepMult + this_materialParameters.metallicRoughnessTexCoord];

        roughness_metallic_pair = SampleTextureBarycentric(intersect_result.barycoords, barycoords_rayDiffs,
                                                           uv_0, uv_1, uv_2, uint(this_materialParameters.metallicRoughnessTexture)).xy;
    }

    // Evaluate
    vec4 color = text_color * this_materialParameters.baseColorFactors * vertex_color;
    vec3 normal = vertex_tangent * text_normal.x + vertex_bitangent * text_normal.y + vertex_normal * text_normal.z;
    vec3 normal_tangent = normalize(vertex_tangent - dot(vertex_tangent, normal) * normal);
    vec3 normal_bitangent = cross(normal, normal_tangent);
    float metallic = roughness_metallic_pair.y;
    float roughness_in = roughness_metallic_pair.x;
    float rough_in_length = RoughnessToLength(roughness_in);
    float rough_length = text_normal_length * rough_in_length;
    float roughness = LengthToRoughness(rough_length);

    // Roughness mollification
    roughness = mix(min_roughness, 1.f, roughness);
    float a_roughness = roughness * roughness;

    // Init return struct in case of early return
    BounceEvaluation return_bounce_evaluation;
    return_bounce_evaluation.light_return = vec3(0.f);
    return_bounce_evaluation.next_bounce_light_factor = vec3(0.f);
    return_bounce_evaluation.light_target = -1;
    return_bounce_evaluation.light_target_contribution = vec3(0.f);

    // Viewvector / Origin
    mat3 normal_to_world_space_mat3 = mat3(normal_tangent, normal_bitangent, normal);
    mat3 world_to_normal_space_mat3 = transpose(normal_to_world_space_mat3);

    vec3 viewVector = -ray_dir;
    vec3 viewVector_normalspace = world_to_normal_space_mat3 * viewVector;
    vec3 origin_pos = pos_0 + edge_1*intersect_result.barycoords.x + edge_2*intersect_result.barycoords.y;

    // If viewvector angle in narrow or negative then return
    if(viewVector_normalspace.z <= DOT_ANGLE_SLACK) {
        return return_bounce_evaluation;
    }

    // Avoid self intersection
    vec3 origin_pos_offseted = RayOffsetFace(origin_pos, face_normal);

    // Avoid self intersecion due to vertex normal
    vec3 vertexNormal_selfintersect_offset = vec3(0.f);
    {
        vec3 pos_0_to_origin = vec3(origin_pos - pos_0);
        vec3 pos_1_to_origin = vec3(origin_pos - pos_1);
        vec3 pos_2_to_origin = vec3(origin_pos - pos_2);

        float max_displacement = 0.f;
        max_displacement = max(max_displacement, -dot(pos_0_to_origin, vertex_normal));
        max_displacement = max(max_displacement, -dot(pos_1_to_origin, vertex_normal));
        max_displacement = max(max_displacement, -dot(pos_2_to_origin, vertex_normal));

        vertexNormal_selfintersect_offset = 1.01f * max_displacement * vertex_normal;
    }

    // Bounce!
    bool is_last_bounce = (ray_depth + 1 == MAX_DEPTH);

    float NdotV = dot(normal, viewVector);

    float color_max = max(color.x, max(color.y, color.z));
    float f0 = mix(0.04f, color_max, metallic);

    float frensel_pow = mix(5.f, 4.f, roughness);
    float e_specular = (f0 + (1.f - f0) * pow(1.f - NdotV, frensel_pow));

    float e_diffuse = color_max * mix((1.f - f0), 0.f, metallic);

    float diffuse_sample_chance = e_diffuse / (e_specular + e_diffuse);

    float u_0 = RandomFloat(rng_state);
    vec3 ray_bounce_normalspace;
    vec3 bounce_halfvector_normalspace;
    if ( u_0 < diffuse_sample_chance ) {
        // Diffuse
        #ifdef USE_INCREASING_MOLLIFICATION
            min_roughness = 0.4f;
        #endif

        ray_bounce_normalspace = RandomCosinWeightedHemi(rng_state);
        bounce_halfvector_normalspace = normalize(ray_bounce_normalspace + viewVector_normalspace);
    } else {
        // Specular
        #ifdef USE_INCREASING_MOLLIFICATION
            min_roughness = min(min_roughness + 0.05f, 0.4f);
        #endif

        bounce_halfvector_normalspace = RandomGXXhalfvector(a_roughness, rng_state);
        ray_bounce_normalspace = reflect(-viewVector_normalspace, bounce_halfvector_normalspace);
    }

    vec3 ray_bounce_dir = normal_to_world_space_mat3 * ray_bounce_normalspace;
    vec3 bounce_halfvector = normal_to_world_space_mat3 * bounce_halfvector_normalspace;
    vec3 bounce_light_factor = vec3(0.f);
    float ray_bounce_PDF = 0.f;
    if (ray_bounce_normalspace.z > DOT_ANGLE_SLACK
     && dot(viewVector_normalspace, bounce_halfvector_normalspace) > DOT_ANGLE_SLACK) {
        float ray_bounce_PDF_cosin = RandomCosinWeightedHemiPDF(ray_bounce_normalspace.z);

        float halfvector_PDF_GGX = RandomGXXhalfvectorPDF(a_roughness, bounce_halfvector_normalspace.z);
        float ray_bounce_PDF_GGX = halfvector_PDF_GGX / (4.f * dot(bounce_halfvector_normalspace, viewVector_normalspace));

        ray_bounce_PDF = ray_bounce_PDF_cosin * diffuse_sample_chance + ray_bounce_PDF_GGX * (1.f - diffuse_sample_chance);

        bounce_light_factor = light_factor  * BRDF(color.xyz, a_roughness, metallic, viewVector, ray_bounce_dir, normal) * dot(ray_bounce_dir, normal) / ray_bounce_PDF;
    }

    // Pick light source
    Reservoir light_reservoir = CreateReservoir();
    vec3 reservoir_light_unshadowed = vec3(0.f);
    vec3 reservoir_ray_dir = vec3(0.f);
    float reservoir_ray_t_max = 0.f;

    float bounce_light_weight = 0.f;
    vec3 bounce_light_unshadowed = vec3(0.f);
    float bounce_light_t_max = INF_DIST;
    uint bounce_light_index = -1;

    /// Check light cones
    for (uint i = 0; i != lightConesIndices_size; ++i) {
        uint this_light_index = uint(lightsCombinations[lightConesIndices_offset + i]);

        vec3 this_light_luminance = lightsParameters[this_light_index].luminance;
        uint this_light_matricesOffset = uint(lightsParameters[this_light_index].matricesOffset);
        float this_light_radius = lightsParameters[this_light_index].radius;

        mat4 light_matrix = model_matrices[this_light_matricesOffset].normalMatrix;
        vec3 light_dir = -vec3(light_matrix[2]);
        vec3 light_dir_tangent = +vec3(light_matrix[0]);
        vec3 light_dir_bitangent = -vec3(light_matrix[1]);

        float angle_tan = this_light_radius;
        vec2 angle_sin_cos = normalize(vec2(angle_tan, 1.f));

        if (dot(light_dir, vertex_normal) > -angle_sin_cos.x) {
            vec3 random_light_dir_zaxis = RandomDirInCone(angle_sin_cos.y, rng_state);
            vec3 random_light_dir = light_dir_bitangent * random_light_dir_zaxis.x + light_dir_tangent * random_light_dir_zaxis.y + light_dir * random_light_dir_zaxis.z;

            float lightcone_PDF = RandomDirInConePDF(angle_sin_cos.y);

            if (dot(random_light_dir, vertex_normal) > DOT_ANGLE_SLACK
             && dot(random_light_dir, normal) > DOT_ANGLE_SLACK) {
                float random_light_dir_PDF_bounce;
                {
                    float NdotL = dot(normal, random_light_dir);

                    vec3 random_light_halfvector = normalize(viewVector + random_light_dir);
                    float NdotLH = dot(normal, random_light_halfvector);
                    float VdotLH = dot(viewVector, random_light_halfvector);

                    float random_light_dir_PDF_cosin = RandomCosinWeightedHemiPDF(NdotL);

                    float random_light_halfvector_PDF_GGX = RandomGXXhalfvectorPDF(a_roughness, NdotLH);
                    float random_light_dir_PDF_GGX = random_light_halfvector_PDF_GGX / (4.f * VdotLH);

                    random_light_dir_PDF_bounce = random_light_dir_PDF_cosin * diffuse_sample_chance + random_light_dir_PDF_GGX * (1.f - diffuse_sample_chance);
                }

                vec3 light_unshadowed = this_light_luminance * light_factor * BRDF(color.xyz, a_roughness, metallic, viewVector, random_light_dir, normal) * dot(normal, random_light_dir) / lightcone_PDF;

                float balance_factor = WeightLightBalance(lightcone_PDF, random_light_dir_PDF_bounce);
                float weight = balance_factor * max(light_unshadowed.r, max(light_unshadowed.g, light_unshadowed.b));

                bool should_swap = UpdateReservoir(weight, light_reservoir, rng_state);
                if (should_swap) {
                    reservoir_light_unshadowed = light_unshadowed;
                    reservoir_ray_dir = random_light_dir;
                    reservoir_ray_t_max = INF_DIST;
                }
            }

            // Bounce dir check for hitting light
            if (dot(ray_bounce_dir, light_dir) > angle_sin_cos.y) {
                vec3 light_unshadowed = this_light_luminance * bounce_light_factor;

                float balance_factor = WeightLightBalance(ray_bounce_PDF, lightcone_PDF);  // lightcone PDF is constant
                float weight = balance_factor * max(light_unshadowed.r, max(light_unshadowed.g, light_unshadowed.b));

                bounce_light_unshadowed = light_unshadowed;
                bounce_light_weight = weight;
                bounce_light_t_max = INF_DIST;
                bounce_light_index = -1;            // Cones are inhittable at BVH
            }
        }
    }

    /// If its the last ray then RIS with bounce ray, else send it to next bounce hit
    if (is_last_bounce) {
        if (bounce_light_weight != 0.f) {   // If bounce ray may hit light then merge
            bool should_swap = UpdateReservoir(bounce_light_weight, light_reservoir, rng_state);
            if (should_swap) {
                reservoir_light_unshadowed = bounce_light_unshadowed;
                reservoir_ray_dir = ray_bounce_dir;
                reservoir_ray_t_max = bounce_light_t_max;
            }
        } else {                            // Else sky is behind
            vec3 skylight_unshadowed = sky_luminance * bounce_light_factor;
            float weight = max(skylight_unshadowed.r, max(skylight_unshadowed.g, skylight_unshadowed.b));

            bool should_swap = UpdateReservoir(weight, light_reservoir, rng_state);
            if (should_swap) {
                reservoir_light_unshadowed = skylight_unshadowed;
                reservoir_ray_dir = ray_bounce_dir;
                reservoir_ray_t_max = INF_DIST;
            }
        }
    } else {
        if (bounce_light_weight != 0.f) {   // If bounce ray may hit return its contibution if finds a light
            float bounce_light_unshadowed_max = max(bounce_light_unshadowed.r, max(bounce_light_unshadowed.g, bounce_light_unshadowed.b));
            return_bounce_evaluation.light_target = bounce_light_index;
            return_bounce_evaluation.light_target_contribution = bounce_light_unshadowed * (bounce_light_weight / bounce_light_unshadowed_max);
        } else {                            // Else sky is behind
            vec3 skylight_unshadowed = sky_luminance * bounce_light_factor;

            return_bounce_evaluation.light_target = -1;
            return_bounce_evaluation.light_target_contribution = skylight_unshadowed;
        }
    }

    // Evaluate light source
    vec3 direct_light = vec3(0.f, 0.f, 0.f);
    if (reservoir_light_unshadowed != vec3(0.f))
    {
        rayQueryEXT query;  // TODO: Change flags
        rayQueryInitializeEXT(query, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, origin_pos_offseted + vertexNormal_selfintersect_offset, 0.0f, reservoir_ray_dir, reservoir_ray_t_max);
        while (rayQueryProceedEXT(query)) {
            if (rayQueryGetIntersectionTypeEXT(query, false) == gl_RayQueryCandidateIntersectionTriangleEXT) {
                ConfirmNonOpaqueIntersection(query);
            }
        }

        if (rayQueryGetIntersectionTypeEXT(query, true) == gl_RayQueryCommittedIntersectionNoneEXT) {
            float reservoir_light_unshadowed_max = max(reservoir_light_unshadowed.r, max(reservoir_light_unshadowed.g, reservoir_light_unshadowed.b));
            vec3 light_contribution = reservoir_light_unshadowed * (light_reservoir.weights_sum / reservoir_light_unshadowed_max);

            #ifndef DEBUG_MIS
                direct_light = reservoir_light_unshadowed * (light_reservoir.weights_sum / reservoir_light_unshadowed_max);
            #else
                float light_contribution_max = max(light_contribution.r, max(light_contribution.g, light_contribution.b));
                direct_light = vec3(light_contribution_max, 0.f, 0.f);
            #endif
        }
    }

    // Return
    return_bounce_evaluation.origin = origin_pos_offseted + vertexNormal_selfintersect_offset;
    return_bounce_evaluation.originDiffs = origin_rayDiffs;

    return_bounce_evaluation.dir = ray_bounce_dir;
    return_bounce_evaluation.dirDiffs = ReflectRayDiffsDir(ray_dirDiffs, bounce_halfvector);
    return_bounce_evaluation.light_return = direct_light;

    if (!is_last_bounce) {
        return_bounce_evaluation.next_bounce_light_factor = bounce_light_factor;
    }

    return return_bounce_evaluation;
}

void main()
{
    uint rng_state = InitRNG(gl_FragCoord.xy, viewportSize, frameIndex);
    float min_roughness = MIN_ROUGHNESS;

    // Read input attachment
    #ifdef MULTISAMPLED_INPUT
        int sample_index = int(frameIndex % MULTISAMPLED_INPUT);
        uvec2 frag_pair = uvec2(subpassLoad(visibilityInput, sample_index));
        uint primitive_instance = frag_pair.x;
        uint triangle_index = frag_pair.y;

        uint alpha_one = uint(frameIndex < MULTISAMPLED_INPUT);

        // TODO: Output stuff
        uint sample_out_mask = 1 << sample_index;
        gl_SampleMask[0] = int(sample_out_mask);

        // If no primitive then sky
        if (primitive_instance == 0) {
            if (alpha_one != 0) {
                color_out = vec4(sky_luminance, 1.f);
            } else {
                color_out = vec4(sky_luminance, 0.f);
            }
            return;
        }

        // Find intersect
        vec2 group_pixel_pos = vec2(0.f);
        uint group_samples_count = 0;

        ivec2 group_sample_triangle_indices_pairs[MULTISAMPLED_INPUT];
        {
            uint indices_offset = primitivesInstancesParameters[primitive_instance].indicesOffset
                + uint(primitivesInstancesParameters[primitive_instance].indicesSetMultiplier) * triangle_index;
            uint p_0_index = uintVerticesBuffers[0].data[indices_offset];
            uint p_1_index = uintVerticesBuffers[0].data[indices_offset + 1];
            uint p_2_index = uintVerticesBuffers[0].data[indices_offset + 2];

            for (int i = 0; i != MULTISAMPLED_INPUT; ++i) {
                uvec2 this_frag_pair = uvec2(subpassLoad(visibilityInput, i));
                uint this_primitive_instance = this_frag_pair.x;
                uint this_triangle_index = this_frag_pair.y;

                if (this_primitive_instance == primitive_instance) {
                    if (this_triangle_index == triangle_index) {
                        group_sample_triangle_indices_pairs[group_samples_count] = ivec2(i, triangle_index);

                        group_pixel_pos += InputSamplesPositions(i);
                        group_samples_count += 1;
                    } else {
                        uint this_indices_offset = primitivesInstancesParameters[primitive_instance].indicesOffset
                            + uint(primitivesInstancesParameters[primitive_instance].indicesSetMultiplier) * this_triangle_index;
                        uint this_p_0_index = uintVerticesBuffers[0].data[this_indices_offset];
                        uint this_p_1_index = uintVerticesBuffers[0].data[this_indices_offset + 1];
                        uint this_p_2_index = uintVerticesBuffers[0].data[this_indices_offset + 2];

                        bool common_point = false;
                        common_point = common_point || this_p_0_index == p_0_index || this_p_1_index == p_1_index || this_p_2_index == p_2_index;
                        common_point = common_point || this_p_0_index == p_1_index || this_p_1_index == p_2_index || this_p_2_index == p_0_index;
                        common_point = common_point || this_p_0_index == p_2_index || this_p_1_index == p_0_index || this_p_2_index == p_1_index;

                        if (common_point) {
                            group_sample_triangle_indices_pairs[group_samples_count] = ivec2(i, this_triangle_index);

                            group_pixel_pos += InputSamplesPositions(i);
                            group_samples_count += 1;
                        }
                    }
                }
            }
            group_pixel_pos /= float(group_samples_count);
        }

        // Find the nearest triangle sample to pixel pos
        uint nearest_triangle_index = -1;
        {
            float min_distanceSq = 8.f;

            for (int i = 0; i != group_samples_count; ++i) {
                vec2 proposed_sample_pos = InputSamplesPositions(group_sample_triangle_indices_pairs[i].x);
                uint proposed_sample_triangle_index = uint(group_sample_triangle_indices_pairs[i].y);

                vec2 distance_vec = proposed_sample_pos - group_pixel_pos;
                float distanceSq = dot(distance_vec, distance_vec);

                if (distanceSq < min_distanceSq) {
                    min_distanceSq = distanceSq;
                    nearest_triangle_index = proposed_sample_triangle_index;
                }
            }
        }

        // Trace ray the primary ray using the nearest sample to mean
        triangle_index = nearest_triangle_index;

        vec2 sample_pixel_offset = group_pixel_pos - vec2(0.5f, 0.5f);

        vec3 ray_dir_center = normalize(vert_normal);
        vec3 ray_dir_center_dx = dFdx(ray_dir_center);
        vec3 ray_dir_center_dy = dFdy(ray_dir_center);

        vec3 ray_dir = normalize(ray_dir_center + sample_pixel_offset.x * ray_dir_center_dx + sample_pixel_offset.y * ray_dir_center_dy);
        RayDiffsDir ray_dirDiffs;
        ray_dirDiffs.dirDx = (1.f - 2.f * abs(sample_pixel_offset.x)) * ray_dir_center_dx;
        ray_dirDiffs.dirDy = (1.f - 2.f * abs(sample_pixel_offset.y)) * ray_dir_center_dy;

        vec3 ray_origin = vec3(0.f);
        RayDiffsOrigin ray_originDiffs;
        ray_originDiffs.originDx = vec3(0.f);
        ray_originDiffs.originDy = vec3(0.f);
    #else
        uvec2 frag_pair = uvec2(subpassLoad(visibilityInput));
        uint primitive_instance = frag_pair.x;
        uint triangle_index = frag_pair.y;

        uint alpha_one = uint(frameIndex == 0);

        if (primitive_instance == 0) {
            if (alpha_one != 0) {
                color_out = vec4(sky_luminance, 1.f);
            } else {
                color_out = vec4(sky_luminance, 0.f);
            }
            return;
        }

        vec3 ray_dir = normalize(vert_normal);
        RayDiffsDir ray_dirDiffs;
        ray_dirDiffs.dirDx = dFdx(ray_dir);
        ray_dirDiffs.dirDy = dFdy(ray_dir);

        vec3 ray_origin = vec3(0.f);
        RayDiffsOrigin ray_originDiffs;
        ray_originDiffs.originDx = vec3(0.f);
        ray_originDiffs.originDy = vec3(0.f);
    #endif

    IntersectTriangleResult intersect_result;

    vec3 light_factor = vec3(1.f);
    vec3 light_sum = vec3(0.f);

    uint light_target = -1;
    vec3 light_hit_contribution = vec3(0.f);

    uint i = 0;
    while(true) {
        BounceEvaluation eval = EvaluateBounce(intersect_result,
                                               primitive_instance, triangle_index,
                                               ray_origin, ray_originDiffs,
                                               ray_dir, ray_dirDiffs,
                                               light_factor, i,
                                               min_roughness, rng_state);

        ray_origin = eval.origin;
        ray_originDiffs = eval.originDiffs;

        ray_dir = eval.dir;
        ray_dirDiffs = eval.dirDiffs;

        light_factor = eval.next_bounce_light_factor;
        light_sum += eval.light_return;

        light_target = eval.light_target;
        light_hit_contribution = eval.light_target_contribution;

        if (light_factor == vec3(0.f))
            break;

        rayQueryEXT query;
        rayQueryInitializeEXT(query, topLevelAS, 0, 0xFF, ray_origin, 0.0f, ray_dir, INF_DIST);
        while (rayQueryProceedEXT(query)) {
            if (rayQueryGetIntersectionTypeEXT(query, false) == gl_RayQueryCandidateIntersectionTriangleEXT) {
                ConfirmNonOpaqueIntersection(query);
            }
        }

        if (rayQueryGetIntersectionTypeEXT(query, true) == gl_RayQueryCommittedIntersectionNoneEXT) {
            #ifndef DEBUG_MIS
                light_sum += light_hit_contribution;
            #else
                float light_hit_contribution_max = max(light_hit_contribution.r, max(light_hit_contribution.g, light_hit_contribution.b));
                light_sum += vec3(0.f, 0.f, light_hit_contribution_max);
            #endif
            break;
        } else {
            // TODO: does it hit light?
            intersect_result.barycoords = rayQueryGetIntersectionBarycentricsEXT(query, true);
            intersect_result.distance = rayQueryGetIntersectionTEXT(query, true);
            primitive_instance = rayQueryGetIntersectionInstanceCustomIndexEXT(query, true) + rayQueryGetIntersectionGeometryIndexEXT(query, true);
            triangle_index = rayQueryGetIntersectionPrimitiveIndexEXT(query, true);
        }

        i++;

        bool has_inf = isinf(light_sum.r) || isinf(light_sum.g) || isinf(light_sum.b);
        bool has_nan = isnan(light_sum.r) || isnan(light_sum.g) || isnan(light_sum.b);
        float max_value = max(light_sum.r, max(light_sum.g, light_sum.b));
        if (has_inf || has_nan || max_value > LIGHT_THRESHOLD) {
            break;
        }
    }

    // Threshold value
    bool has_inf = isinf(light_sum.r) || isinf(light_sum.g) || isinf(light_sum.b);
    bool has_nan = isnan(light_sum.r) || isnan(light_sum.g) || isnan(light_sum.b);
    if (has_inf || has_nan) {
        light_sum = vec3(0.f);
    }
    float max_value = max(light_sum.r, max(light_sum.g, light_sum.b));
    if (max_value > LIGHT_THRESHOLD) {
        float factor = LIGHT_THRESHOLD / max_value;
        light_sum *= factor;
    }

    // Color out
    if (alpha_one != 0) {
        color_out = vec4(light_sum, 1.f);
    } else {
        color_out = vec4(light_sum, 0.f);
    }
}