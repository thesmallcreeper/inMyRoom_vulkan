#ifndef FILE_CPP_INTERFACE

#ifdef ENABLE_CPP_INTERFACE

    #define VEC4        alignas(16) glm::vec4
    #define VEC3        alignas(16) glm::vec3
    #define VEC2        alignas(8) glm::vec2

    #define FLOAT_T     alignas(4) float
    #define UINT_T      alignas(4) uint32_t
    #define UINT16_T    alignas(2) uint16_t
    #define UINT8_T     alignas(1) uint8_t
#else
    #define VEC4        vec4
    #define VEC3        vec3
    #define VEC2        vec2

    #define FLOAT_T     float
    #define UINT_T      uint
    #define UINT16_T    uint16_t
    #define UINT8_T     uint8_t
#endif

#define FILE_CPP_INTERFACE
#endif