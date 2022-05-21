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
#include "common/sRGBencode.glsl"

#include "NRD/helpers/NRD_packNormalRoughness.glsl"
#include "NRD/helpers/REBLUR_getNormalizedHitDist.glsl"

#define MAX_DEPTH 3
#define DOT_ANGLE_SLACK 0.0087265f      // cos(89.5 degs)
#define MIN_ROUGHNESS 0.02f
#define USE_INCREASING_MOLLIFICATION
#define LIGHT_THRESHOLD 0.5e6f

// #define DEBUG_MIS

#define INF_DIST 100000.f
#define FP16_MAX 65504.f

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
layout (input_attachment_index = 0, set = 5, binding = 0) uniform usubpassInput visibilityInput;


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

#define SPECULAR_DIFFUSE_EVAL
#include "common/evaluateBounce.glsl"

void main()
{
    uint rng_state = InitRNG(gl_FragCoord.xy, viewportSize, frameIndex);
    float min_roughness = MIN_ROUGHNESS;

    uvec2 frag_pair = uvec2(subpassLoad(visibilityInput));
    uint primitive_instance = frag_pair.x;
    uint triangle_index = frag_pair.y;

    if (primitive_instance == 0) {
        diffuse_out = vec4(vec3(0.f), 0.f);
        specular_out = vec4(vec3(0.f), 0.f);
        normalRoughness_out = vec4(0.f);
        colorMetalness_out = vec4(0.f);
        return;
    }

    const vec3 primary_ray_dir = normalize(vert_normal);

    vec3 ray_dir = primary_ray_dir;
    RayDiffsDir ray_dirDiffs;
    ray_dirDiffs.dirDx = dFdx(primary_ray_dir);
    ray_dirDiffs.dirDy = dFdy(primary_ray_dir);

    vec3 ray_origin = vec3(0.f);
    RayDiffsOrigin ray_originDiffs;
    ray_originDiffs.originDx = vec3(0.f);
    ray_originDiffs.originDy = vec3(0.f);

    IntersectTriangleResult intersect_result;

    float view_Z_linear = INF_DIST;
    float first_bounce_T = INF_DIST;

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

            view_Z_linear = eval.origin.z;

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
        rayQueryInitializeEXT(query, topLevelAS, 0, 0xFF, ray_origin, 0.0f, ray_dir, INF_DIST);
        while (rayQueryProceedEXT(query)) {
            if (rayQueryGetIntersectionTypeEXT(query, false) == gl_RayQueryCandidateIntersectionTriangleEXT) {
                ConfirmNonOpaqueIntersection(query);
            }
        }

        if (i == 0) {
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
            // TODO: does it hit light?
            intersect_result.barycoords = rayQueryGetIntersectionBarycentricsEXT(query, true);
            intersect_result.distance = rayQueryGetIntersectionTEXT(query, true);
            primitive_instance = rayQueryGetIntersectionInstanceCustomIndexEXT(query, true) + rayQueryGetIntersectionGeometryIndexEXT(query, true);
            triangle_index = rayQueryGetIntersectionPrimitiveIndexEXT(query, true);
        }

        i++;

        vec3 light_sum = light_sum_specular + light_sum_diffuse;
        bool has_inf = isinf(light_sum.r) || isinf(light_sum.g) || isinf(light_sum.b);
        bool has_nan = isnan(light_sum.r) || isnan(light_sum.g) || isnan(light_sum.b);
        float max_value = max(light_sum.r, max(light_sum.g, light_sum.b));
        if (has_inf || has_nan || max_value > LIGHT_THRESHOLD) {
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
    if (max_value > LIGHT_THRESHOLD) {
        float factor = LIGHT_THRESHOLD / max_value;
        light_sum_specular *= factor;
        light_sum_diffuse *= factor;
    }

    // Exposure TODO!
    light_sum_diffuse /= FP16_FACTOR;
    light_sum_specular /= FP16_FACTOR;

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
    float diffuse_normHitDist = REBLUR_FrontEnd_GetNormHitDist(first_bounce_T, view_Z_linear);
    float specular_normHitDist = REBLUR_FrontEnd_GetNormHitDist(first_bounce_T, view_Z_linear, roughness);

    // TODO: Move vector

    // Color out
    diffuse_out = vec4(min(demod_diffuse, FP16_MAX), diffuse_normHitDist);
    specular_out = vec4(min(demod_specular, FP16_MAX), specular_normHitDist);
    normalRoughness_out = NRD_FrontEnd_PackNormalRoughness(normal, roughness);
    colorMetalness_out = sRGBencode(vec4(baseColor, metallic));
    motionVec_out = vec4(0.f);
    linearViewZ_out = view_Z_linear;
}