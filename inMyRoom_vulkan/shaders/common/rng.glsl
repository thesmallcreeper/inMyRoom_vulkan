#ifndef FILE_RNG

#include "common/brdf.glsl"

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif


// Ray tracing gems 2, p. 170
uint JenkinsHash(uint x) {
    x += (x << 10);
    x ^= (x >> 6);
    x += (x << 3);
    x ^= (x >> 11);
    x += (x << 15);
    return x;
}

uint InitRNG(vec2 pixel_coords, uvec2 resolution, uint frame) {
    uvec2 pixel = uvec2(floor(pixel_coords));
    uint rngState = JenkinsHash(pixel.x + pixel.y * resolution.x) ^ JenkinsHash(frame);
    return JenkinsHash(rngState);
}

float _uintToFloat(uint x) {
    return uintBitsToFloat(uint(0x3f800000) | (x >> 9)) - 1.f;
}

uint XORshift(inout uint rngState) {
    rngState ^= (rngState << 13);
    rngState ^= (rngState >> 17);
    rngState ^= (rngState << 5);
    return rngState;
}

float RandomFloat(inout uint rngState) {
    return _uintToFloat(XORshift(rngState));
}

vec3 RandomDirInCone(float cosMaxTheta, inout uint rngState) {
    float u_0 = RandomFloat(rngState);
    float u_1 = RandomFloat(rngState);

    float cosTheta = (1.f - u_0) + u_0 * cosMaxTheta;
    float sinTheta = sqrt(1.f - min(cosTheta * cosTheta, 1.f));
    float phi = u_1 * 2.f * M_PI;

    vec3 return_vector;
    return_vector.x = cos(phi) * sinTheta;
    return_vector.y = sin(phi) * sinTheta;
    return_vector.z = cosTheta;

    return return_vector;
}

float RandomDirInConePDF(float cosMaxTheta) {
    return 1.f / (2.f * M_PI * (1.f - cosMaxTheta));
}

vec3 RandomGXXhalfvector(float a, inout uint rngState) {

    float u_0 = RandomFloat(rngState);
    float u_1 = RandomFloat(rngState);

    float cosTheta = sqrt((1.f - u_0) / ((a*a - 1.f) * u_0 + 1.f));
    float sinTheta = sqrt(1.f - min(cosTheta * cosTheta, 1.f));

    float cosPhi = cos(2.f * M_PI * u_1);
    float sinPhi = sin(2.f * M_PI * u_1);

    vec3 return_vector;
    return_vector.x = cosPhi * sinTheta;
    return_vector.y = sinPhi * sinTheta;
    return_vector.z = cosTheta;

    return return_vector;
}

float RandomGXXhalfvectorPDF(float a, float NdotH) {
    return GGXdistribution(a, NdotH) * NdotH;
}

vec3 RandomCosinWeightedHemi(inout uint rngState) {
    float u_0 = RandomFloat(rngState);
    float u_1 = RandomFloat(rngState);

    float cosPhi = cos(2.f * M_PI * u_1);
    float sinPhi = sin(2.f * M_PI * u_1);
    float u_0_sqrt = sqrt(u_0);

    vec3 return_vector;
    return_vector.x = u_0_sqrt * cosPhi;
    return_vector.y = u_0_sqrt * sinPhi;
    return_vector.z = sqrt(1.f - u_0);

    return return_vector;
}

float RandomCosinWeightedHemiPDF(float NdotH) {
    return NdotH / M_PI;
}

#define FILE_RNG
#endif