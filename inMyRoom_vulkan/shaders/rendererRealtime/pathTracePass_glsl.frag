#version 460

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_shader_16bit_storage : require
#extension GL_EXT_shader_8bit_storage : require
#extension GL_EXT_ray_query : require
#extension GL_EXT_control_flow_attributes : require

#include "common/defines.h"
#include "common/structs/ModelMatrices.h"
#include "common/structs/MaterialParameters.h"
#include "common/structs/PrimitiveInstanceParameters.h"
#include "common/structs/LightParameters.h"
#include "common/samplesPosition.glsl"
#include "common/sRGBencode.glsl"
#include "common/visibilityBufferPack.glsl"

#include "NRD/helpers/NRD_packNormalRoughness.glsl"

#ifdef DENOISER_REBLUR
#include "NRD/helpers/REBLUR_getNormalizedHitDist.glsl"
#endif

#define MAX_DEPTH 3
#define MIN_RUSSIAN_DEPTH 2
#define RUSSIAN_CHANCE_FACTOR 2.f

#define DOT_ANGLE_SLACK 0.00390625f      // cos(89.78 degs)
#define MIN_ROUGHNESS 0.02f
#define USE_INCREASING_MOLLIFICATION
#define LIGHT_THRESHOLD 0.5e6f
#define MIN_DIFFUSE_CHANCE 0.05f
#define MIN_SPECULAR_CHANCE 0.15f
#define BAYER_1ST_BOUNCE

// #define DEBUG_MIS

#define INF_DIST 100000.f
#define FP16_MAX 65504.f
#define FLT_EPSILON 1.192092896e-07f

//
// In
layout( location = 0 ) in vec3 vert_normal;

//
// Out
layout( location = 0 ) out vec4 diffuse_out;
layout( location = 1 ) out vec4 specular_out;
layout( location = 2 ) out vec4 normalRoughness_out;
layout( location = 3 ) out vec4 colorMetalness_out;
layout( location = 4 ) out vec4 motionVec_out;
layout( location = 5 ) out float linearViewZ_out;
#ifdef MORPHOLOGICAL_MSAA
layout( location = 6 ) out uint morphologicalMask_out;
#endif

//
// Descriptors

/// 0, 0
layout( set = 0 , binding = 0 ) uniform projectionMatrixBuffer
{
    mat4 viewMatrix;
    mat4 inverseViewMatrix;
    mat4 projectionMatrix;
};

// 0, 1
layout( set = 0 , binding = 1 ) uniform prevProjectionMatrixBuffer
{
    mat4 prevViewMatrix;
    mat4 prevInverseViewMatrix;
    mat4 prevProjectionMatrix;
};

/// 1, 0
layout( std430, set = 1 , binding = 0 ) readonly buffer worldSpaceMatricesBufferDescriptor
{
    ModelMatrices model_matrices[MATRICES_COUNT];
};

layout( std430, set = 1 , binding = 1 ) readonly buffer prevWorldSpaceMatricesBufferDescriptor
{
    ModelMatrices prev_model_matrices[MATRICES_COUNT];
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
layout( std430, set = 3, binding = 0 ) readonly buffer uintPrevVerticesBuffersDescriptors
{
    uint data[];
} uintPrevVerticesBuffers [];

layout( std430, set = 3, binding = 0 ) readonly buffer vec2prevVerticesBuffersDescriptors
{
    vec2 data[];
} vec2prevVerticesBuffers [];

layout( std430, set = 3, binding = 0 ) readonly buffer vec4prevVerticesBuffersDescriptors
{
    vec4 data[];
} vec4prevVerticesBuffers [];

/// 4, 0
layout (std430, set = 4, binding = 0) readonly buffer materialsParametersBufferDescriptor
{
    MaterialParameters materialsParameters[MATERIALS_PARAMETERS_COUNT];
};

/// 4, 1
layout (set = 4, binding = 1) uniform sampler2D textures[TEXTURES_COUNT];

/// 5, 0
layout (set = 5, binding = 0) readonly buffer primitivesInstancesBufferDescriptor
{
    PrimitiveInstanceParameters primitivesInstancesParameters[INSTANCES_COUNT];
};

/// 6, 0
#ifdef MORPHOLOGICAL_MSAA
layout (input_attachment_index = 0, set = 6, binding = 0) uniform usubpassInputMS visibilityInput;
#else
layout (input_attachment_index = 0, set = 6, binding = 0) uniform usubpassInput visibilityInput;
#endif

/// 7, 0
layout(set = 7, binding = 0) uniform accelerationStructureEXT topLevelAS;

/// 8, 0
layout (set = 8, binding = 0) readonly buffer lightsParametersBufferDescriptor
{
    LightParameters lightsParameters[MAX_LIGHTS_COUNT];
};

// 8, 1
layout (set = 8, binding = 1) readonly buffer lightsCombinationsBufferDescriptor
{
    uint16_t lightsCombinations[MAX_COMBINATIONS_SIZE];
};

//
// Push constants
layout (push_constant) uniform PushConstants
{
    layout(offset = 0)  vec3 sky_luminance;
    layout(offset = 16) uvec2 viewportSize;
    layout(offset = 24) uint frameCount;
    layout(offset = 28) uint lightConesIndices_offset;
    layout(offset = 32) uint lightConesIndices_size;
    layout(offset = 36) float HDR_factor;
};

#include "common/evaluateBounce.glsl"

void main()
{
    float light_threshold = min(LIGHT_THRESHOLD, 0.99f * FP16_MAX * HDR_factor);

    uint rng_state = InitRNG(gl_FragCoord.xy, viewportSize, frameCount);
    float min_roughness = MIN_ROUGHNESS;

    vec2 sample_pixel_offset = vec2(0.f);

    #ifdef MORPHOLOGICAL_MSAA

    // Cluster samples
    struct SamplesClusterDirty {
        uint visibility_pair;
        uint mask;
        bool is_latest;
    } samplesClusters_dirty[MORPHOLOGICAL_MSAA];
    [[unroll]] for (int i = 0; i != MORPHOLOGICAL_MSAA; ++i) {
        samplesClusters_dirty[i].visibility_pair = uint(subpassLoad(visibilityInput, i));
        samplesClusters_dirty[i].is_latest = true;

        uint cluster_mask = 0;
        [[unroll]] for (int j = 0; j != i; ++j) {
            if (samplesClusters_dirty[i].visibility_pair == samplesClusters_dirty[j].visibility_pair) {
                cluster_mask = samplesClusters_dirty[j].mask;
                samplesClusters_dirty[j].is_latest = false;
            }
        }
        samplesClusters_dirty[i].mask = cluster_mask | uint(1) << i;
    }

    struct SamplesCluster {
        uint visibility_pair;
        uint mask;
    } samplesClusters[MORPHOLOGICAL_MSAA];
    uint samplesClusters_count = 0;
    [[unroll]] for (int i = 0; i != MORPHOLOGICAL_MSAA; ++i) {
        if (samplesClusters_dirty[i].is_latest == true) {
            samplesClusters[samplesClusters_count].visibility_pair = samplesClusters_dirty[i].visibility_pair;
            samplesClusters[samplesClusters_count].mask = samplesClusters_dirty[i].mask;
            samplesClusters_count++;
        }
    }

    // Init
    struct SamplesGroupInfoStruct {
        uint mask;
        uint primitiveInstance;
        uvec3 p;
    };

    // Create sample groups
    SamplesGroupInfoStruct samplesGroupInfos [MORPHOLOGICAL_MSAA];
    [[unroll]] for (int i = 0; i != MORPHOLOGICAL_MSAA && i != samplesClusters_count; ++i) {
        uvec2 frag_pair = UnpackVisibilityBuffer(samplesClusters[i].visibility_pair);
        uint primitive_instance = frag_pair.x;
        uint triangle_index = frag_pair.y;

        uint indices_offset = primitivesInstancesParameters[primitive_instance].indicesOffset
            + uint(primitivesInstancesParameters[primitive_instance].indicesSetMultiplier) * triangle_index;
        uvec3 p_indices = uvec3(uintVerticesBuffers[0].data[indices_offset], uintVerticesBuffers[0].data[indices_offset + 1], uintVerticesBuffers[0].data[indices_offset + 2]);

        SamplesGroupInfoStruct best_sampleGroup;
        best_sampleGroup.mask = 0;
        best_sampleGroup.primitiveInstance = primitive_instance;
        best_sampleGroup.p = p_indices;

        [[unroll]] for (int j = i - 1; j != -1 && best_sampleGroup.mask == 0 ; j--) {
            if (samplesGroupInfos[j].primitiveInstance == primitive_instance) {
                if (any(equal(samplesGroupInfos[j].p, p_indices))
                 || any(equal(samplesGroupInfos[j].p, p_indices.yzx))
                 || any(equal(samplesGroupInfos[j].p, p_indices.zxy)))
                {
                    best_sampleGroup = samplesGroupInfos[j];
                    samplesGroupInfos[j].primitiveInstance = 0;
                }
                else // "Paranoid" search
                {
                    uint pos_descriptorIndex = uint(primitivesInstancesParameters[primitive_instance].positionDescriptorIndex);
                    uint pos_offset = primitivesInstancesParameters[primitive_instance].positionOffset;

                    uint normal_descriptorIndex = uint(primitivesInstancesParameters[primitive_instance].normalDescriptorIndex);
                    uint normal_offset = primitivesInstancesParameters[primitive_instance].normalOffset;

                    #ifdef MLAA_CHECK_UV

                    uint uv_descriptorIndex = uint(primitivesInstancesParameters[primitive_instance].texcoordsDescriptorIndex);
                    uint uv_offset = primitivesInstancesParameters[primitive_instance].texcoordsOffset;

                    uint uv_stepMult = uint(primitivesInstancesParameters[primitive_instance].texcoordsStepMultiplier);
                    uint material_index = uint(primitivesInstancesParameters[primitive_instance].material);
                    uint baseColor_TexCoord = materialsParameters[material_index].baseColorTexCoord;
                    #endif

                    vec3 group_poss[3] = { vec3(vec4verticesBuffers[pos_descriptorIndex].data[pos_offset + samplesGroupInfos[j].p[0]]),
                    vec3(vec4verticesBuffers[pos_descriptorIndex].data[pos_offset + samplesGroupInfos[j].p[1]]),
                    vec3(vec4verticesBuffers[pos_descriptorIndex].data[pos_offset + samplesGroupInfos[j].p[2]]) };

                    vec3 sample_poss[3] = { vec3(vec4verticesBuffers[pos_descriptorIndex].data[pos_offset + p_indices[0]]),
                    vec3(vec4verticesBuffers[pos_descriptorIndex].data[pos_offset + p_indices[1]]),
                    vec3(vec4verticesBuffers[pos_descriptorIndex].data[pos_offset + p_indices[2]]) };

                    uint group_p = -1;
                    uint sample_p = -1;

                    // Crosscheck points
                    [[unroll]] for (int n = 0; n != 3; ++n) {
                        [[unroll]] for (int m = 0; m != 3; ++m)  {
                            if (all(lessThan(abs(group_poss[n] - sample_poss[m]), vec3(FLT_EPSILON)))) {
                                group_p = samplesGroupInfos[j].p[n];
                                sample_p = p_indices[m];
                            }
                        }
                    }

                    // Then check color UV and normal at common point
                    if (group_p != -1) {
                        vec3 group_normal = vec3(vec4verticesBuffers[normal_descriptorIndex].data[normal_offset + group_p]);
                        vec3 sample_normal = vec3(vec4verticesBuffers[normal_descriptorIndex].data[normal_offset + sample_p]);

                        #ifdef MLAA_CHECK_UV

                        vec2 group_uv = vec2verticesBuffers[uv_descriptorIndex].data[uv_offset + group_p * uv_stepMult + baseColor_TexCoord];
                        vec2 sample_uv = vec2verticesBuffers[uv_descriptorIndex].data[uv_offset + sample_p * uv_stepMult + baseColor_TexCoord];
                        #endif

                        if (dot(group_normal, sample_normal) > 0.99f
                        #ifdef MLAA_CHECK_UV
                        && all(lessThan(abs(group_uv - sample_uv), vec2(FLT_EPSILON) ))
                        #endif
                        ) {
                            best_sampleGroup = samplesGroupInfos[j];
                            samplesGroupInfos[j].primitiveInstance = 0;
                        }
                    }
                }
            }
        }

        samplesGroupInfos[i].primitiveInstance = best_sampleGroup.primitiveInstance;
        samplesGroupInfos[i].mask = best_sampleGroup.mask | samplesClusters[i].mask;
        samplesGroupInfos[i].p = best_sampleGroup.p;
    }

    // Find the biggest group
    SamplesGroupInfoStruct selected_group;
    selected_group.mask = 0;
    uint selected_group_samplesCount = 0;
    for (int i = 0; i != MORPHOLOGICAL_MSAA && i != samplesClusters_count; ++i) {
        uint popcnt = bitCount(samplesGroupInfos[i].mask);
        if (samplesGroupInfos[i].primitiveInstance != 0 && popcnt > selected_group_samplesCount) {
            selected_group = samplesGroupInfos[i];
            selected_group_samplesCount = popcnt;
        }
    }

    // Get mean of samples positions and find the nearest triangle
    uint first_bounce_primitive_instance;
    uint first_bounce_triangle_index;
    if (selected_group.mask != 0) {
        morphologicalMask_out = selected_group.mask;

        vec2 group_pixel_pos = vec2(0.f);
        for (int i = 0; i != MORPHOLOGICAL_MSAA; ++i) {
            uint sampleANDmask = uint(1) << i;
            uint isSampleInGroup = selected_group.mask & sampleANDmask;
            if (isSampleInGroup != 0) {
                group_pixel_pos += InputSamplesPositions(i);
            }
        }
        group_pixel_pos /= float(selected_group_samplesCount);
        sample_pixel_offset = group_pixel_pos - vec2(0.5f, 0.5f);

        float min_distanceSq = 8.f;
        int sample_triangle_from = 0;
        for (int i = 0; i != MORPHOLOGICAL_MSAA; ++i) {
            uint sampleANDmask = uint(1) << i;
            uint isSampleInGroup = selected_group.mask & sampleANDmask;
            if (isSampleInGroup != 0) {
                vec2 proposed_sample_pos = InputSamplesPositions(i);

                vec2 distance_vec = proposed_sample_pos - group_pixel_pos;
                float distanceSq = dot(distance_vec, distance_vec);

                if (distanceSq < min_distanceSq) {
                    min_distanceSq = distanceSq;
                    sample_triangle_from = i;
                }
            }
        }
        first_bounce_primitive_instance = selected_group.primitiveInstance;
        first_bounce_triangle_index = UnpackVisibilityBuffer(uint(subpassLoad(visibilityInput, sample_triangle_from))).y;

    } else {
        morphologicalMask_out = ( uint(1) << MORPHOLOGICAL_MSAA ) - 1;

        first_bounce_primitive_instance = 0;
        first_bounce_triangle_index = 0;
    }

    #else
    uvec2 frag_pair = UnpackVisibilityBuffer(uint(subpassLoad(visibilityInput)));

    uint first_bounce_primitive_instance = frag_pair.x;
    uint first_bounce_triangle_index = frag_pair.y;
    #endif

    uint primitive_instance = first_bounce_primitive_instance;
    uint triangle_index = first_bounce_triangle_index;

    if (primitive_instance == 0) {
        diffuse_out = vec4(vec3(0.f), 0.f);
        specular_out = vec4(vec3(0.f), 0.f);
        normalRoughness_out = vec4(1.f);
        colorMetalness_out = vec4(0.f);
        motionVec_out = vec4(0.f);
        linearViewZ_out = INF_DIST;
        return;
    }

    vec3 ray_dir_center = normalize(vert_normal);
    vec3 ray_dir_center_dx = dFdx(ray_dir_center);
    vec3 ray_dir_center_dy = dFdy(ray_dir_center);

    vec3 primary_ray_dir = normalize(ray_dir_center + sample_pixel_offset.x * ray_dir_center_dx + sample_pixel_offset.y * ray_dir_center_dy);

    vec3 ray_dir = primary_ray_dir;
    RayDiffsDir ray_dirDiffs;
    ray_dirDiffs.dirDx = (1.f - 2.f * abs(sample_pixel_offset.x)) * ray_dir_center_dx;
    ray_dirDiffs.dirDy = (1.f - 2.f * abs(sample_pixel_offset.y)) * ray_dir_center_dy;

    vec3 ray_origin = vec3(0.f);
    RayDiffsOrigin ray_originDiffs;
    ray_originDiffs.originDx = vec3(0.f);
    ray_originDiffs.originDy = vec3(0.f);

    IntersectTriangleResult intersect_result;

    IntersectTriangleResult first_hit_intersect_result;
    first_hit_intersect_result.distance = INF_DIST;
    first_hit_intersect_result.barycoords = vec2(0.3f);

    bool first_bounce_wasDiffuse = true;
    float first_bounce_T = 0.f;

    vec3 baseColor = vec3(0.f);
    float metallic = 1.f;
    vec3 normal = vec3(0.f, 0.f, -1.f);
    float roughness = 0.f;

    vec3 light_factor = vec3(1.f);
    vec3 indirect_specular_factor = vec3(0.f);

    vec3 light_sum_specular = vec3(0.f);
    vec3 light_sum_diffuse  = vec3(0.f);

    uint light_target = -1;
    vec3 light_target_contribution_specular = vec3(0.f);
    vec3 light_target_contribution_diffuse = vec3(0.f);

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

        light_target = eval.light_target;
        light_target_contribution_specular = eval.light_target_contribution_specular;
        light_target_contribution_diffuse = eval.light_target_contribution_diffuse;

        light_factor = eval.next_bounce_light_factor_specular + eval.next_bounce_light_factor_diffuse;
        if (i == 0) {
            baseColor = eval.baseColor;
            metallic = eval.metallic;
            normal = eval.normal;
            roughness = eval.roughness;

            first_hit_intersect_result = intersect_result;

            light_sum_specular += eval.light_return_specular;
            light_sum_diffuse += eval.light_return_diffuse;

            vec3 NaN_fix = vec3(equal(light_factor, vec3(0.f)));
            indirect_specular_factor = eval.next_bounce_light_factor_specular / (light_factor + NaN_fix);
        } else {
            vec3 light_returned = eval.light_return_specular + eval.light_return_diffuse;
            light_sum_specular += indirect_specular_factor * light_returned;
            light_sum_diffuse += (vec3(1.f) - indirect_specular_factor) * light_returned;
        }

        if (light_factor == vec3(0.f))
        break;

        rayQueryEXT query;
        rayQueryInitializeEXT(query, topLevelAS, 0, MESH_MASK | LIGHT_MASK, ray_origin, 0.0f, ray_dir, INF_DIST);
        while (rayQueryProceedEXT(query)) {
            if (rayQueryGetIntersectionTypeEXT(query, false) == gl_RayQueryCandidateIntersectionTriangleEXT) {
                ConfirmNonOpaqueIntersection(query);
            }
        }

        if (i == 0) {
            first_bounce_wasDiffuse = eval.isDiffuseSample;
            first_bounce_T = rayQueryGetIntersectionTEXT(query, true);
        }

        if (rayQueryGetIntersectionTypeEXT(query, true) == gl_RayQueryCommittedIntersectionNoneEXT) {
            #ifndef DEBUG_MIS
            if (i == 0) {
                light_sum_specular += light_target_contribution_specular;
                light_sum_diffuse += light_target_contribution_diffuse;
            } else {
                vec3 light_hit = light_target_contribution_specular + light_target_contribution_diffuse;
                light_sum_specular += indirect_specular_factor * light_hit;
                light_sum_diffuse += (vec3(1.f) - indirect_specular_factor) * light_hit;
            }
            #else
            if (i == 0) {
                float light_hit_contribution_specular_lum = Luminance(light_target_contribution_specular);
                light_sum_specular += vec3(0.f, 0.f, light_hit_contribution_specular_lum);
                float light_hit_contribution_diffuse_lum = Luminance(light_target_contribution_diffuse);
                light_sum_diffuse +=  vec3(0.f, 0.f, light_hit_contribution_diffuse_lum);
            } else {
                vec3 light_hit = light_target_contribution_specular + light_target_contribution_diffuse;
                float light_hit_lum = Luminance(light_hit);
                light_sum_specular += indirect_specular_factor * vec3(0.f, 0.f, light_hit_lum);
                light_sum_diffuse += (vec3(1.f) - indirect_specular_factor) * vec3(0.f, 0.f, light_hit_lum);
            }
            #endif
            break;
        } else {
            intersect_result.barycoords = rayQueryGetIntersectionBarycentricsEXT(query, true);
            intersect_result.distance = rayQueryGetIntersectionTEXT(query, true);
            primitive_instance = rayQueryGetIntersectionInstanceCustomIndexEXT(query, true) + rayQueryGetIntersectionGeometryIndexEXT(query, true);
            triangle_index = rayQueryGetIntersectionPrimitiveIndexEXT(query, true);

            uint light_offset = uint(primitivesInstancesParameters[primitive_instance].light);
            if (light_offset != -1 && light_offset == light_target) {
                #ifndef DEBUG_MIS
                if (i == 0) {
                    light_sum_specular += light_target_contribution_specular;
                    light_sum_diffuse += light_target_contribution_diffuse;
                } else {
                    vec3 light_hit = light_target_contribution_specular + light_target_contribution_diffuse;
                    light_sum_specular += indirect_specular_factor * light_hit;
                    light_sum_diffuse += (vec3(1.f) - indirect_specular_factor) * light_hit;
                }
                #else
                if (i == 0) {
                    float light_hit_contribution_specular_lum = Luminance(light_target_contribution_specular);
                    light_sum_specular += vec3(0.f, 0.f, light_hit_contribution_specular_lum);
                    float light_hit_contribution_diffuse_lum = Luminance(light_target_contribution_diffuse);
                    light_sum_diffuse +=  vec3(0.f, 0.f, light_hit_contribution_diffuse_lum);
                } else {
                    vec3 light_hit = light_target_contribution_specular + light_target_contribution_diffuse;
                    float light_hit_lum = Luminance(light_hit);
                    light_sum_specular += indirect_specular_factor * vec3(0.f, 0.f, light_hit_lum);
                    light_sum_diffuse += (vec3(1.f) - indirect_specular_factor) * vec3(0.f, 0.f, light_hit_lum);
                }
                #endif
                break;
            }
        }

        i++;

        vec3 light_sum = light_sum_specular + light_sum_diffuse;
        bool has_inf = isinf(light_sum.r) || isinf(light_sum.g) || isinf(light_sum.b);
        bool has_nan = isnan(light_sum.r) || isnan(light_sum.g) || isnan(light_sum.b);
        float max_value = max(light_sum.r, max(light_sum.g, light_sum.b));
        if (has_inf || has_nan || max_value > light_threshold) {
            break;
        }
    }

    // Threshold value
    vec3 light_sum = light_sum_specular + light_sum_diffuse;
    bool has_inf = isinf(light_sum.r) || isinf(light_sum.g) || isinf(light_sum.b);
    bool has_nan = isnan(light_sum.r) || isnan(light_sum.g) || isnan(light_sum.b);
    if (has_inf || has_nan) {
        light_sum = vec3(0.f);
    }
    float max_value = max(light_sum.r, max(light_sum.g, light_sum.b));
    if (max_value > light_threshold) {
        float factor = light_threshold / max_value;
        light_sum_specular *= factor;
        light_sum_diffuse *= factor;
    }

    // Exposure
    light_sum_diffuse /= HDR_factor;
    light_sum_specular /= HDR_factor;

    // Check if visible normal
    bool visible_normal = dot(-primary_ray_dir, normal) > DOT_ANGLE_SLACK;

    // Get linear Z
    float view_Z_linear = dot(primary_ray_dir, vec3(0.f, 0.f, 1.f)) * first_hit_intersect_result.distance;

    // De-modulate diffuse specular
    vec3 c_diff = mix(baseColor.rgb, vec3(0.f), metallic);
    vec3 f0 = mix(vec3(0.04f), baseColor.rgb, metallic);

    c_diff = max(c_diff, 0.001f);
    vec3 demod_diffuse = light_sum_diffuse * (M_PI / c_diff);

    float NdotV = dot(normal, -primary_ray_dir);
    vec3 envTerm = EnvironmentTerm(f0, NdotV, roughness * roughness);
    envTerm = max(envTerm, 0.001f);
    vec3 demod_specular = light_sum_specular / envTerm;

    // Get hit distances
    #ifdef DENOISER_REBLUR
    float diffuse_hitDist = REBLUR_FrontEnd_GetNormHitDist(first_bounce_wasDiffuse ? first_bounce_T : 0.f, view_Z_linear);
    float specular_hitDist = REBLUR_FrontEnd_GetNormHitDist(first_bounce_wasDiffuse ? 0.f : first_bounce_T, view_Z_linear, roughness);
    #else
    float diffuse_hitDist = first_bounce_wasDiffuse ? first_bounce_T : 0.f;
    float specular_hitDist = first_bounce_wasDiffuse ? 0.f : first_bounce_T;
    #endif

    // Transform normal to world-space
    vec3 normal_worldspace = vec3(inverseViewMatrix * vec4(normal, 0.f));

    // Get motion vector
    vec3 pos = primary_ray_dir * first_hit_intersect_result.distance;

    vec3 prev_pos = pos;
    uint prev_matrixOffset = uint(primitivesInstancesParameters[first_bounce_primitive_instance].prevMatricesOffset);
    if ( prev_matrixOffset != -1) {
        mat4 prev_matrix = prev_model_matrices[prev_matrixOffset].positionMatrix;

        uint indices_offset = primitivesInstancesParameters[first_bounce_primitive_instance].indicesOffset
        + uint(primitivesInstancesParameters[first_bounce_primitive_instance].indicesSetMultiplier) * first_bounce_triangle_index;
        uint p_0_index = uintVerticesBuffers[0].data[indices_offset];
        uint p_1_index = uintVerticesBuffers[0].data[indices_offset + 1];
        uint p_2_index = uintVerticesBuffers[0].data[indices_offset + 2];

        uint pos_descriptorIndex = uint(primitivesInstancesParameters[first_bounce_primitive_instance].positionDescriptorIndex);
        uint pos_offset = primitivesInstancesParameters[first_bounce_primitive_instance].positionOffset;

        vec3 pos_0 = vec3(prev_matrix * vec4prevVerticesBuffers[pos_descriptorIndex].data[pos_offset + p_0_index]);
        vec3 pos_1 = vec3(prev_matrix * vec4prevVerticesBuffers[pos_descriptorIndex].data[pos_offset + p_1_index]);
        vec3 pos_2 = vec3(prev_matrix * vec4prevVerticesBuffers[pos_descriptorIndex].data[pos_offset + p_2_index]);

        vec3 edge_1 = pos_1 - pos_0;
        vec3 edge_2 = pos_2 - pos_0;
        vec3 prev_pos_prevSpace = pos_0 + first_hit_intersect_result.barycoords.x * edge_1 + first_hit_intersect_result.barycoords.y * edge_2;

        prev_pos = vec3(viewMatrix * prevInverseViewMatrix * vec4(prev_pos_prevSpace, 1.f));
    }

    vec3 motion_vec = vec3(inverseViewMatrix * vec4(prev_pos - pos, 0.f));

    // Color out
    diffuse_out = vec4(min(demod_diffuse, FP16_MAX), visible_normal ? diffuse_hitDist : 0.f);
    specular_out = vec4(min(demod_specular, FP16_MAX), visible_normal ? specular_hitDist : 0.f);
    normalRoughness_out = NRD_FrontEnd_PackNormalRoughness(normal_worldspace, roughness);
    colorMetalness_out = sRGBencode(vec4(baseColor, metallic));
    motionVec_out = vec4(motion_vec, 0.f);
    linearViewZ_out = visible_normal ? view_Z_linear : INF_DIST;
}