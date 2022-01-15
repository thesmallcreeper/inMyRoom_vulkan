#version 450

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_shader_16bit_storage : require
#extension GL_EXT_shader_8bit_storage : require

struct MaterialParameters
{
    vec4 baseColorFactors;
    uint baseColorTexture;
    uint baseColorTexCoord;

    float alphaCutoff;
};

struct PrimitiveInstanceParameters {
    uint indicesOffset;      // uint32_t
    uint positionOffset;     // glm::vec4
    uint normalOffset;       // glm::vec4
    uint tangentOffset;      // glm::vec4
    uint texcoordsOffset;    // glm::vec2
    uint colorOffset;        // glm::vec4

    uint16_t matricesOffset;
    uint16_t material;
    uint16_t positionDescriptorIndex;
    uint16_t normalDescriptorIndex;
    uint16_t tangentDescriptorIndex;
    uint16_t texcoordsDescriptorIndex;
    uint16_t colorDescriptorIndex;

    uint8_t indicesSetMultiplier;
    uint8_t texcoordsStepMultiplier;
    uint8_t colorStepMultiplier;
};

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
    mat4 projectionMatrix;
};

/// 1, 0
layout( std430, set = 1 , binding = 0 ) readonly buffer worldSpaceMatricesBufferDescriptor
{
    mat4 matrices[MATRICES_COUNT];
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

//
// Push constants

void main()
{
    vec3 ray_dir = normalize(vert_normal.xyz);

    uvec2 frag_pair = uvec2(subpassLoad(visibilityInput));
    uint frag_primitiveInstance = frag_pair.x;
    uint frag_triangleIndex = frag_pair.y;

    uint indices_offset = primitivesInstancesParameters[frag_primitiveInstance].indicesOffset + uint(primitivesInstancesParameters[frag_primitiveInstance].indicesSetMultiplier) * frag_triangleIndex;
    uint p_0_index = uintVerticesBuffers[0].data[indices_offset];
    uint p_1_index = uintVerticesBuffers[0].data[indices_offset + 1];
    uint p_2_index = uintVerticesBuffers[0].data[indices_offset + 2];

    uint matrixOffset = uint(primitivesInstancesParameters[frag_primitiveInstance].matricesOffset);
    mat4x4 pos_matrix = viewMatrix * matrices[matrixOffset];

    uint pos_descriptorIndex = uint(primitivesInstancesParameters[frag_primitiveInstance].positionDescriptorIndex);
    uint pos_offset = primitivesInstancesParameters[frag_primitiveInstance].positionOffset;
    vec4 vert_0 = pos_matrix * vec4verticesBuffers[pos_descriptorIndex].data[pos_offset + p_0_index];
    vec4 vert_1 = pos_matrix * vec4verticesBuffers[pos_descriptorIndex].data[pos_offset + p_1_index];
    vec4 vert_2 = pos_matrix * vec4verticesBuffers[pos_descriptorIndex].data[pos_offset + p_2_index];

    vec3 edge1 = vec3(vert_1 - vert_0);
    vec3 edge2 = vec3(vert_2 - vert_0);

    vec3 p = cross(ray_dir, edge2);
    float det = dot(edge1, p);

    vec2 baryPosition;
    baryPosition.x = dot(-vec3(vert_0), p);

    vec3 perped = cross(-vec3(vert_0), edge1);
    baryPosition.y = dot(ray_dir, perped);

    baryPosition /= det;

    uint material_index = uint(primitivesInstancesParameters[frag_primitiveInstance].material);
    MaterialParameters this_materialParameters = materialsParameters[material_index];

    uint uv_descriptorIndex = uint(primitivesInstancesParameters[frag_primitiveInstance].texcoordsDescriptorIndex);
    uint uv_offset = primitivesInstancesParameters[frag_primitiveInstance].texcoordsOffset;
    uint uv_stepMult = uint(primitivesInstancesParameters[frag_primitiveInstance].texcoordsStepMultiplier);
    vec2 uv_0 = vec2verticesBuffers[uv_descriptorIndex].data[uv_offset + p_0_index * uv_stepMult + this_materialParameters.baseColorTexCoord];
    vec2 uv_1 = vec2verticesBuffers[uv_descriptorIndex].data[uv_offset + p_1_index * uv_stepMult + this_materialParameters.baseColorTexCoord];
    vec2 uv_2 = vec2verticesBuffers[uv_descriptorIndex].data[uv_offset + p_2_index * uv_stepMult + this_materialParameters.baseColorTexCoord];

    vec2 uv = (1.f - baryPosition.x - baryPosition.y) * uv_0 + baryPosition.x * uv_1 + baryPosition.y * uv_2;

    uint base_color_texture_index = this_materialParameters.baseColorTexture;
    vec4 text_color = texture(textures[base_color_texture_index], uv).rgba * this_materialParameters.baseColorFactors;

    color_out = text_color;
}