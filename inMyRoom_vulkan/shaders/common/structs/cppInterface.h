#ifndef FILE_CPP_INTERFACE

#ifdef ENABLE_CPP_INTERFACE
    #include <cstddef>
    #include "glm/matrix.hpp"

    #define MAT4_T      alignas(16) glm::mat4
    #define MAT3_T      alignas(16) glm::mat3
    #define MAT2_T      alignas(8)  glm::mat2
    #define MAT4_T_C    glm::mat4
    #define MAT3_T_C    glm::mat3
    #define MAT2_T_C    glm::mat2

    #define VEC4        alignas(16) glm::vec4
    #define VEC3        alignas(16) glm::vec3
    #define VEC2        alignas(8) glm::vec2
    #define VEC4_C      glm::vec4
    #define VEC3_C      glm::vec3
    #define VEC2_C      glm::vec2

    #define IVEC4       alignas(16) glm::ivec4
    #define IVEC3       alignas(16) glm::ivec3
    #define IVEC2       alignas(8) glm::ivec2
    #define IVEC4_C     glm::ivec4
    #define IVEC3_C     glm::ivec3
    #define IVEC2_C     glm::ivec2

    #define FLOAT_T     alignas(4) float
    #define UINT_T      alignas(4) uint32_t
    #define UINT16_T    alignas(2) uint16_t
    #define UINT8_T     alignas(1) uint8_t

    #define DOT         glm::dot
    #define MAX         std::max
    #define MIN         std::min

    #define SQRT        std::sqrt

#else
    #define MAT4_T      mat4
    #define MAT3_T      mat3
    #define MAT2_T      mat2
    #define MAT4_T_C    mat4
    #define MAT3_T_C    mat3
    #define MAT2_T_C    mat2

    #define VEC4        vec4
    #define VEC3        vec3
    #define VEC2        vec2
    #define VEC4_C      vec4
    #define VEC3_C      vec3
    #define VEC2_C      vec2

    #define IVEC4       ivec4
    #define IVEC3       ivec3
    #define IVEC2       ivec2
    #define IVEC4_C     ivec4
    #define IVEC3_C     ivec3
    #define IVEC2_C     ivec2

    #define FLOAT_T     float
    #define UINT_T      uint
    #define UINT16_T    uint16_t
    #define UINT8_T     uint8_t

    #define DOT         dot
    #define MAX         max
    #define MIN         min

    #define SQRT        sqrt
#endif

#define FILE_CPP_INTERFACE
#endif