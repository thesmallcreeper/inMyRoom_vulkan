#ifndef FILE_EVALUATE_BOUNCE

#include "common/intersectTriangle.glsl"
#include "common/rayOffsetFace.glsl"
#include "common/RoughnessLengthMap.h"
#include "common/rng.glsl"
#include "common/brdf.glsl"
#include "common/reservoir.glsl"
#include "common/luminance.glsl"
#include "common/environmentTerm.glsl"
#include "common/bayer.glsl"

struct BounceEvaluation {
    vec3 baseColor;
    float metallic;
    vec3 normal;
    float roughness;

    vec3 origin;
    RayDiffsOrigin originDiffs;

    vec3 dir;
    RayDiffsDir dirDiffs;

    vec3 next_bounce_light_factor_specular;
    vec3 next_bounce_light_factor_diffuse;
    vec3 light_return_specular;
    vec3 light_return_diffuse;

    uint light_target;
    vec3 light_target_contribution_specular;
    vec3 light_target_contribution_diffuse;
    
    bool isDiffuseSample;
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

float WindowFunction(float dist, float range) {
    float dist_range_ratio_pow = pow(dist / range, 4.f);
    float clamped_sqrt = clamp(1.0f - dist_range_ratio_pow, 0.f, 1.f);
    return clamped_sqrt * clamped_sqrt;
}

BounceEvaluation EvaluateBounce(inout IntersectTriangleResult intersect_result,
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

    vec3 c_diff = mix(color.rgb, vec3(0.f), metallic);
    vec3 f0 = mix(vec3(0.04f), color.rgb, metallic);

    // Roughness mollification
    roughness = mix(min_roughness, 1.f, roughness);
    float a_roughness = roughness * roughness;

    // Init return struct in case of early return
    BounceEvaluation return_bounce_evaluation;
    return_bounce_evaluation.baseColor = vec3(color);
    return_bounce_evaluation.metallic = metallic;
    return_bounce_evaluation.normal = normal;
    return_bounce_evaluation.roughness = roughness;
    return_bounce_evaluation.light_target = -1;
    return_bounce_evaluation.light_return_specular = vec3(0.f);
    return_bounce_evaluation.light_return_diffuse = vec3(0.f);
    return_bounce_evaluation.next_bounce_light_factor_specular = vec3(0.f);
    return_bounce_evaluation.next_bounce_light_factor_diffuse = vec3(0.f);
    return_bounce_evaluation.light_target_contribution_specular = vec3(0.f);
    return_bounce_evaluation.light_target_contribution_diffuse = vec3(0.f);
    return_bounce_evaluation.isDiffuseSample = true;

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
    float NdotV = dot(normal, viewVector);
    float e_specular = Luminance(EnvironmentTerm(f0, NdotV, a_roughness));

    float e_diffuse = Luminance(c_diff);

    float diffuse_sample_chance = e_diffuse / (e_specular + e_diffuse);

    #ifdef MIN_DIFFUSE_CHANCE
        diffuse_sample_chance = metallic == 1.f ? 0.f : max(MIN_DIFFUSE_CHANCE, diffuse_sample_chance);
    #endif

    #ifdef MIN_SPECULAR_CHANCE
        diffuse_sample_chance = min(1.f - MIN_SPECULAR_CHANCE, diffuse_sample_chance);
    #endif

    float u_0 = 0.f;
    #ifdef BAYER_1ST_BOUNCE
        if (ray_depth == 0) {
            u_0 = BayerNoise(gl_FragCoord.xy, frameCount, rng_state);
        } else {
            u_0 = RandomFloat(rng_state);
        }
    #else
        u_0 = RandomFloat(rng_state);
    #endif
    vec3 ray_bounce_normalspace;
    vec3 bounce_halfvector_normalspace;
    if ( u_0 < diffuse_sample_chance ) {
        // Diffuse
        #ifdef USE_INCREASING_MOLLIFICATION
        min_roughness = 0.4f;
        #endif

        ray_bounce_normalspace = RandomCosinWeightedHemi(rng_state);
        bounce_halfvector_normalspace = normalize(ray_bounce_normalspace + viewVector_normalspace);

        return_bounce_evaluation.isDiffuseSample = true;
    } else {
        // Specular
        #ifdef USE_INCREASING_MOLLIFICATION
        min_roughness = min(min_roughness + 0.05f, 0.4f);
        #endif

        bounce_halfvector_normalspace = RandomGXXhalfvector(a_roughness, rng_state);
        ray_bounce_normalspace = reflect(-viewVector_normalspace, bounce_halfvector_normalspace);

        return_bounce_evaluation.isDiffuseSample = false;
    }

    vec3 ray_bounce_dir = normal_to_world_space_mat3 * ray_bounce_normalspace;
    vec3 bounce_halfvector = normal_to_world_space_mat3 * bounce_halfvector_normalspace;
    vec3 bounce_light_factor_specular = vec3(0.f);
    vec3 bounce_light_factor_diffuse = vec3(0.f);

    float ray_bounce_PDF = 0.f;
    bool is_last_bounce = false;
    bool is_bounce_valid = false;
    if (ray_bounce_normalspace.z > DOT_ANGLE_SLACK
    && dot(viewVector_normalspace, bounce_halfvector_normalspace) > DOT_ANGLE_SLACK) {
        float ray_bounce_PDF_cosin = RandomCosinWeightedHemiPDF(ray_bounce_normalspace.z);

        float halfvector_PDF_GGX = RandomGXXhalfvectorPDF(a_roughness, bounce_halfvector_normalspace.z);
        float ray_bounce_PDF_GGX = halfvector_PDF_GGX / (4.f * dot(bounce_halfvector_normalspace, viewVector_normalspace));

        ray_bounce_PDF = ray_bounce_PDF_cosin * diffuse_sample_chance + ray_bounce_PDF_GGX * (1.f - diffuse_sample_chance);

        BRDFresult BRDF_eval = BRDF(c_diff, f0, a_roughness, viewVector, ray_bounce_dir, normal);

        bounce_light_factor_specular = light_factor * BRDF_eval.f_specular * dot(ray_bounce_dir, normal) / ray_bounce_PDF;
        bounce_light_factor_diffuse = light_factor * BRDF_eval.f_diffuse * dot(ray_bounce_dir, normal) / ray_bounce_PDF;

        is_bounce_valid = true;
    } else {
        is_last_bounce = true;
        is_bounce_valid = false;
    }

    // Russian roulette
    is_last_bounce = is_last_bounce || (ray_depth + 1 == MAX_DEPTH);
    float path_continue_factor = 1.f;
    #ifdef MIN_RUSSIAN_DEPTH
        if (!is_last_bounce && ray_depth + 1 >= MIN_RUSSIAN_DEPTH) {
            vec3 bounce_light_factor = bounce_light_factor_specular + bounce_light_factor_diffuse;
            float bounce_light_luminance_factor = Luminance(bounce_light_factor);

            float path_continue_chance = min(1.f, RUSSIAN_CHANCE_FACTOR * bounce_light_luminance_factor);
            if (path_continue_chance < RandomFloat(rng_state)) {
                is_last_bounce = true;
            }

            // Will multiply with 1/path_continue_chance for next non-light bounce
            path_continue_factor = 1.f / path_continue_chance;
        }
    #endif

    // Pick light source
    Reservoir light_reservoir = CreateReservoir();
    vec3 reservoir_light_unshadowed_specular = vec3(0.f);
    vec3 reservoir_light_unshadowed_diffuse = vec3(0.f);
    vec3 reservoir_ray_dir = vec3(0.f);
    float reservoir_ray_t_max = 0.f;

    float bounce_light_weight = 0.f;
    vec3 bounce_light_unshadowed_specular = vec3(0.f);
    vec3 bounce_light_unshadowed_diffuse = vec3(0.f);
    float bounce_light_t_max = INF_DIST;
    uint bounce_light_offset = -1;

    /// Check light cones
    for (uint i = 0; i != lightConesIndices_size; ++i) {
        uint this_light_offset = uint(lightsCombinations[lightConesIndices_offset + i]);

        vec3 this_light_luminance = lightsParameters[this_light_offset].luminance;
        uint this_light_matricesOffset = uint(lightsParameters[this_light_offset].matricesOffset);
        float this_light_radius = lightsParameters[this_light_offset].radius;

        mat4 light_matrix = model_matrices[this_light_matricesOffset].normalMatrix;
        vec3 light_dir = -vec3(light_matrix[2]);
        vec3 light_dir_tangent = +vec3(light_matrix[0]);
        vec3 light_dir_bitangent = -vec3(light_matrix[1]);

        float angle_tan = this_light_radius;
        vec2 angle_sin_cos = normalize(vec2(angle_tan, 1.f));

        if (dot(light_dir, vertex_normal) > -angle_sin_cos.x
         && dot(light_dir, normal) > -angle_sin_cos.x) {
            vec3 random_light_dir_zaxis = RandomDirInCone(angle_sin_cos.y, rng_state);
            vec3 random_light_dir = light_dir_bitangent * random_light_dir_zaxis.x + light_dir_tangent * random_light_dir_zaxis.y + light_dir * random_light_dir_zaxis.z;

            float lightcone_PDF = RandomDirInConePDF(angle_sin_cos.y);
            float light_illuminance = Luminance(this_light_luminance) / lightcone_PDF;

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

                BRDFresult BRDF_eval = BRDF(c_diff, f0, a_roughness, viewVector, random_light_dir, normal);
                vec3 light_unshadowed_specular = this_light_luminance * light_factor * BRDF_eval.f_specular * dot(normal, random_light_dir) / lightcone_PDF;
                vec3 light_unshadowed_diffuse = this_light_luminance * light_factor * BRDF_eval.f_diffuse * dot(normal, random_light_dir) / lightcone_PDF;
                vec3 light_unshadowed = light_unshadowed_specular + light_unshadowed_diffuse;

                float balance_factor = WeightLightBalance(lightcone_PDF, random_light_dir_PDF_bounce);
                float weight = balance_factor * Luminance(light_unshadowed);

                bool should_swap = UpdateReservoir(weight, light_reservoir, rng_state);
                if (should_swap) {
                    reservoir_light_unshadowed_specular = light_unshadowed_specular;
                    reservoir_light_unshadowed_diffuse = light_unshadowed_diffuse;

                    reservoir_ray_dir = random_light_dir;
                    reservoir_ray_t_max = INF_DIST;
                }
            }

            // Bounce dir check for hitting light
            if (is_bounce_valid
             && dot(ray_bounce_dir, light_dir) > angle_sin_cos.y) {
                bounce_light_unshadowed_specular = this_light_luminance * bounce_light_factor_specular;
                bounce_light_unshadowed_diffuse = this_light_luminance * bounce_light_factor_diffuse;
                vec3 light_unshadowed = bounce_light_unshadowed_specular + bounce_light_unshadowed_diffuse;

                float balance_factor = WeightLightBalance(ray_bounce_PDF, lightcone_PDF);  // lightcone PDF is constant
                float weight = balance_factor * Luminance(light_unshadowed);

                bounce_light_weight = weight;
                bounce_light_t_max = INF_DIST;
                bounce_light_offset = -1;            // Cones are inhittable at BVH
            }
        }
    }
    /// Check local lights
    uint local_lights_count = uint(primitivesInstancesParameters[primitive_instance].lightsCombinationsCount);
    uint local_lights_offset = uint(primitivesInstancesParameters[primitive_instance].lightsCombinationsOffset);
    for (uint i = 0; i != local_lights_count; ++i) {
        uint this_light_index = uint(lightsCombinations[local_lights_offset + i]);

        vec3 this_light_luminance = lightsParameters[this_light_index].luminance;
        uint this_light_matricesOffset = uint(lightsParameters[this_light_index].matricesOffset);
        float this_light_radius = lightsParameters[this_light_index].radius;
        float this_light_range = lightsParameters[this_light_index].range;

        mat4 light_matrix_pos = model_matrices[this_light_matricesOffset].positionMatrix;
        mat4 light_matrix_normalized = model_matrices[this_light_matricesOffset].normalMatrix;

        vec3 light_pos = vec3(light_matrix_pos[3]);
        vec3 light_vec = light_pos - (origin_pos_offseted + vertexNormal_selfintersect_offset);
        float light_dist = length(light_vec);
        vec3 light_dir = light_vec / light_dist;

        vec2 angle_sin_cos = vec2(this_light_radius / light_dist, 0.f);
        angle_sin_cos.y = sqrt(1.f - angle_sin_cos.x * angle_sin_cos.x);

        if (dot(light_dir, vertex_normal) > -angle_sin_cos.x
         && dot(light_dir, normal) > -angle_sin_cos.x
         && light_dist < this_light_range) {
            vec3 light_dir_tangent;
            if (abs(dot(light_dir, vec3(light_matrix_normalized[0]))) < 0.5f) {    // 1/sqrt(3) = 0.5774
                vec3 proposed_dir_tangent = vec3(light_matrix_normalized[0]);
                light_dir_tangent = normalize(proposed_dir_tangent - dot(proposed_dir_tangent, light_dir) * light_dir);
            } else if (abs(dot(light_dir, vec3(light_matrix_normalized[1]))) < 0.5f) {
                vec3 proposed_dir_tangent = vec3(light_matrix_normalized[1]);
                light_dir_tangent = normalize(proposed_dir_tangent - dot(proposed_dir_tangent, light_dir) * light_dir);
            } else {
                vec3 proposed_dir_tangent = vec3(light_matrix_normalized[2]);
                light_dir_tangent = normalize(proposed_dir_tangent - dot(proposed_dir_tangent, light_dir) * light_dir);
            }
            vec3 light_dir_bitangent = cross(light_dir, light_dir_tangent);

            vec3 random_light_dir_zaxis = RandomDirInCone(angle_sin_cos.y, rng_state);
            vec3 random_light_dir = light_dir_bitangent * random_light_dir_zaxis.x + light_dir_tangent * random_light_dir_zaxis.y + light_dir * random_light_dir_zaxis.z;

            float lightcone_PDF = RandomDirInConePDF(angle_sin_cos.y);
            float window_coeff = WindowFunction(light_dist, this_light_range);
            float light_illuminance = window_coeff * Luminance(this_light_luminance) / lightcone_PDF;

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

                BRDFresult BRDF_eval = BRDF(c_diff, f0, a_roughness, viewVector, random_light_dir, normal);
                vec3 light_unshadowed_specular = window_coeff * this_light_luminance * light_factor * BRDF_eval.f_specular * dot(normal, random_light_dir) / lightcone_PDF;
                vec3 light_unshadowed_diffuse = window_coeff * this_light_luminance * light_factor * BRDF_eval.f_diffuse * dot(normal, random_light_dir) / lightcone_PDF;
                vec3 light_unshadowed = light_unshadowed_specular + light_unshadowed_diffuse;

                float balance_factor = WeightLightBalance(lightcone_PDF, random_light_dir_PDF_bounce);
                float weight = balance_factor * Luminance(light_unshadowed);

                bool should_swap = UpdateReservoir(weight, light_reservoir, rng_state);
                if (should_swap) {
                    reservoir_light_unshadowed_specular = light_unshadowed_specular;
                    reservoir_light_unshadowed_diffuse = light_unshadowed_diffuse;

                    reservoir_ray_dir = random_light_dir;
                    reservoir_ray_t_max = light_dist / random_light_dir_zaxis.z;
                }
            }

            // Bounce dir check for hitting light
            float bounce_dart_dist = light_dist / dot(ray_bounce_dir, light_dir);
            if (is_bounce_valid
             && dot(ray_bounce_dir, light_dir) > angle_sin_cos.y
             && bounce_dart_dist < bounce_light_t_max) {
                bounce_light_unshadowed_specular = window_coeff * this_light_luminance * bounce_light_factor_specular;
                bounce_light_unshadowed_diffuse = window_coeff * this_light_luminance * bounce_light_factor_diffuse;
                vec3 light_unshadowed = bounce_light_unshadowed_specular + bounce_light_unshadowed_diffuse;

                float balance_factor = WeightLightBalance(ray_bounce_PDF, lightcone_PDF);  // lightcone PDF is constant
                float weight = balance_factor * Luminance(light_unshadowed);

                bounce_light_weight = weight;
                bounce_light_t_max = bounce_dart_dist;
                bounce_light_offset = this_light_index;
            }
        }
    }

    // If its the last ray then RIS with bounce ray, else send it to next bounce hit
    if (is_last_bounce) {
        if (bounce_light_weight != 0.f) {   // If bounce ray may hit light then merge
            bool should_swap = UpdateReservoir(bounce_light_weight, light_reservoir, rng_state);
            if (should_swap) {
                reservoir_light_unshadowed_specular = bounce_light_unshadowed_specular;
                reservoir_light_unshadowed_diffuse = bounce_light_unshadowed_diffuse;

                reservoir_ray_dir = ray_bounce_dir;
                reservoir_ray_t_max = bounce_light_t_max;
            }
        } else {                            // Else sky is behind
            vec3 skylight_unshadowed_specular = sky_luminance * bounce_light_factor_specular;
            vec3 skylight_unshadowed_diffuse = sky_luminance * bounce_light_factor_diffuse;
            vec3 skylight_unshadowed = skylight_unshadowed_specular + skylight_unshadowed_diffuse;

            float weight = Luminance(skylight_unshadowed);

            bool should_swap = UpdateReservoir(weight, light_reservoir, rng_state);
            if (should_swap) {
                reservoir_light_unshadowed_specular = skylight_unshadowed_specular;
                reservoir_light_unshadowed_diffuse = skylight_unshadowed_diffuse;

                reservoir_ray_dir = ray_bounce_dir;
                reservoir_ray_t_max = INF_DIST;
            }
        }
    } else {
        if (bounce_light_weight != 0.f) {   // If bounce ray may hit return its contibution if finds a light
            return_bounce_evaluation.light_target = bounce_light_offset;

            vec3 bounce_light_unshadowed = bounce_light_unshadowed_specular + bounce_light_unshadowed_diffuse;
            float bounce_light_unshadowed_lum = Luminance(bounce_light_unshadowed);
            return_bounce_evaluation.light_target_contribution_specular = bounce_light_unshadowed_specular * (bounce_light_weight / bounce_light_unshadowed_lum);
            return_bounce_evaluation.light_target_contribution_diffuse = bounce_light_unshadowed_diffuse * (bounce_light_weight / bounce_light_unshadowed_lum);
        } else {                            // Else sky is behind
            return_bounce_evaluation.light_target = -1;

            return_bounce_evaluation.light_target_contribution_specular = sky_luminance * bounce_light_factor_specular;
            return_bounce_evaluation.light_target_contribution_diffuse = sky_luminance * bounce_light_factor_diffuse;
        }
    }

    // Evaluate light source
    vec3 direct_light_specular = vec3(0.f, 0.f, 0.f);
    vec3 direct_light_diffuse = vec3(0.f, 0.f, 0.f);
    vec3 reservoir_light_unshadowed = reservoir_light_unshadowed_specular + reservoir_light_unshadowed_diffuse;

    if (reservoir_light_unshadowed != vec3(0.f))
    {
        rayQueryEXT query;
        rayQueryInitializeEXT(query, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT, MESH_MASK, origin_pos_offseted + vertexNormal_selfintersect_offset, 0.0f, reservoir_ray_dir, reservoir_ray_t_max);
        while (rayQueryProceedEXT(query)) {
            if (rayQueryGetIntersectionTypeEXT(query, false) == gl_RayQueryCandidateIntersectionTriangleEXT) {
                ConfirmNonOpaqueIntersection(query);
            }
        }

        if (rayQueryGetIntersectionTypeEXT(query, true) == gl_RayQueryCommittedIntersectionNoneEXT) {
            float reservoir_light_unshadowed_lum = Luminance(reservoir_light_unshadowed);
            #ifndef DEBUG_MIS
                direct_light_specular = reservoir_light_unshadowed_specular * (light_reservoir.weights_sum / reservoir_light_unshadowed_lum);
                direct_light_diffuse = reservoir_light_unshadowed_diffuse * (light_reservoir.weights_sum / reservoir_light_unshadowed_lum);
            #else
                vec3 light_contribution_specular = reservoir_light_unshadowed_specular * (light_reservoir.weights_sum / reservoir_light_unshadowed_lum);
                float light_contribution_specular_lum = Luminance(light_contribution_specular);
                direct_light_specular = vec3(light_contribution_specular_lum, 0.f, 0.f);

                vec3 light_contribution_diffuse = reservoir_light_unshadowed_diffuse * (light_reservoir.weights_sum / reservoir_light_unshadowed_lum);
                float light_contribution_diffuse_lum = Luminance(light_contribution_diffuse);
                direct_light_diffuse = vec3(light_contribution_diffuse_lum, 0.f, 0.f);
            #endif
        }
    }

    // Return
    return_bounce_evaluation.origin = origin_pos_offseted + vertexNormal_selfintersect_offset;
    return_bounce_evaluation.originDiffs = origin_rayDiffs;

    return_bounce_evaluation.dir = ray_bounce_dir;
    return_bounce_evaluation.dirDiffs = ReflectRayDiffsDir(ray_dirDiffs, bounce_halfvector);

    return_bounce_evaluation.light_return_specular = direct_light_specular;
    return_bounce_evaluation.light_return_diffuse = direct_light_diffuse;

    if (!is_last_bounce) {
        return_bounce_evaluation.next_bounce_light_factor_specular = path_continue_factor * bounce_light_factor_specular;
        return_bounce_evaluation.next_bounce_light_factor_diffuse = path_continue_factor * bounce_light_factor_diffuse;
    }

    return return_bounce_evaluation;
}

#define FILE_EVALUATE_BOUNCE
#endif