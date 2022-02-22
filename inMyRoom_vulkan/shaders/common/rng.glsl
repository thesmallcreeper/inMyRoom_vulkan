
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// Ray tracing gems 2, p. 170
uint JenkinsHash(uint x) {
    x += x << 10;
    x ^= x >> 6;
    x += x << 3;
    x ^= x >> 11;
    x += x << 15;
    return x;
}

uint InitRNG(vec2 pixel_coords, uvec2 resolution, uint frame) {
    uvec2 pixel = uvec2(pixel_coords * resolution);
    uint rngState = (pixel.x + pixel.y * resolution.x) ^ JenkinsHash(frame);
    return JenkinsHash(rngState);
}

float _uintToFloat(uint x) {
    return uintBitsToFloat(uint(0x3f800000) | (x >> 9)) - 1.f;
}

uint XORshift(inout uint rngState) {
    rngState ^= rngState << 13;
    rngState ^= rngState >> 17;
    rngState ^= rngState << 5;
    return rngState;
}

float RandomFloat(inout uint rngState) {
    return _uintToFloat(XORshift(rngState));
}

vec3 RandomDirInCone(float cosMaxTheta, inout uint rngState) {
    float u_0 = RandomFloat(rngState);
    float u_1 = RandomFloat(rngState);

    vec3 return_vector;
    float cosTheta = (1 - u_0) + u_0 * cosMaxTheta;
    float sinTheta = sqrt(1 - cosTheta * cosTheta);
    float phi = u_1 * 2 * M_PI;
    return_vector.x = cos(phi) * sinTheta;
    return_vector.y = sin(phi) * sinTheta;
    return_vector.z = cosTheta;

    return return_vector;
}

float RandomDirInConePDF(float cosMaxTheta) {
    return 1 / (2.f * M_PI * (1.f - cosMaxTheta));
}