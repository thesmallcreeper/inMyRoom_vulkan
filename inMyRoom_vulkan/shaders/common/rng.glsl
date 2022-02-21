
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