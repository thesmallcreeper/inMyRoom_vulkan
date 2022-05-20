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
#include "common/samplesPosition.glsl"

#define MAX_DEPTH 3
#define DOT_ANGLE_SLACK 0.0087265f      // cos(89.5 degs)
#define MIN_ROUGHNESS 0.02f
#define USE_INCREASING_MOLLIFICATION
#define LIGHT_THRESHOLD 0.5e6f

// #define SPECULAR_DIFFUSE_EVAL
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

#include "common/evaluateBounce.glsl"

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
    uint light_target = -1;

    vec3 light_factor = vec3(1.f);
    vec3 light_sum = vec3(0.f);
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

        light_target = eval.light_target;

        #ifdef SPECULAR_DIFFUSE_EVAL
            light_factor = eval.next_bounce_light_factor_specular + eval.next_bounce_light_factor_diffuse;
            light_sum += eval.light_return_specular + eval.light_return_diffuse;
            light_hit_contribution = eval.light_target_contribution_specular + eval.light_target_contribution_diffuse;
        #else
            light_factor = eval.next_bounce_light_factor;
            light_sum += eval.light_return;
            light_hit_contribution = eval.light_target_contribution;
        #endif

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
                float light_hit_contribution_lum = Luminance(light_hit_contribution);
                light_sum += vec3(0.f, 0.f, light_hit_contribution_lum);
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