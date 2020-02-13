#pragma once

#define TINYGLTF_TYPE_VEC2 (2)
#define TINYGLTF_TYPE_VEC3 (3)
#define TINYGLTF_TYPE_VEC4 (4)
#define TINYGLTF_TYPE_MAT2 (32 + 2)
#define TINYGLTF_TYPE_MAT3 (32 + 3)
#define TINYGLTF_TYPE_MAT4 (32 + 4)
#define TINYGLTF_TYPE_SCALAR (64 + 1)
#define TINYGLTF_TYPE_VECTOR (64 + 4)
#define TINYGLTF_TYPE_MATRIX (64 + 16)


enum class glTFtype
{
    type_scalar = TINYGLTF_TYPE_SCALAR,
    type_vector = TINYGLTF_TYPE_VECTOR,
    type_matrix = TINYGLTF_TYPE_MATRIX,
    type_vec2 = TINYGLTF_TYPE_VEC2,
    type_vec3 = TINYGLTF_TYPE_VEC3,
    type_vec4 = TINYGLTF_TYPE_VEC4,
    type_mat2 = TINYGLTF_TYPE_MAT2,
    type_mat3 = TINYGLTF_TYPE_MAT3,
    type_mat4 = TINYGLTF_TYPE_MAT4
};

enum class glTFmode
{
    points = 0,
    line = 1,
    line_loop = 2,
    line_strip = 3,
    triangles = 4,
    triangle_strip = 5,
    triangle_fan = 6
};

enum class glTFcomponentType
{
    type_byte = 5120,
    type_unsigned_byte = 5121,
    type_short = 5122,
    type_unsigned_short = 5123,
    type_int = 5124,
    type_unsigned_int = 5125,
    type_float = 5126,
    type_double = 5130
};

enum class glTFparameterType
{
    type_byte = 5120,
    type_unsigned_byte = 5121,
    type_short = 5122,
    type_unsigned_short = 5123,
    type_int = 5124,
    type_unsigned_int = 5125,
    type_float = 5126,
    type_double = 5130,

    type_float_vec2 = 35664,
    type_float_vec3 = 35665,
    type_float_vec4 = 35666,

    type_int_vec2 = 35667,
    type_int_vec3 = 35668,
    type_int_vec4 = 35669,

    type_bool = 35670,
    type_bool_vec2 = 35671,
    type_bool_vec3 = 35672,
    type_bool_vec4 = 35673,

    type_float_mat2 = 35674,
    type_float_mat3 = 35675,
    type_float_mat4 = 35676,

    type_sampler_2D = 35678
};

enum class glTFsamplerMagFilter
{
    nearest = 9728,
    linear = 9729
};

enum class glTFsamplerMinFilter
{
    nearest = 9728,
    linear = 9729,
    nearest_mipmap_nearest = 9984,
    linear_mipmap_nearest = 9985,
    nearest_mipmap_linear = 9986,
    linear_mipmap_linear = 9987
};

enum class glTFsamplerWrap
{
    clamp_to_edge = 33071,
    mirrored_repeat = 33648,
    repeat = 10497
};

