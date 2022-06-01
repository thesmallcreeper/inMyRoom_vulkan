#ifndef FILE_BAYER

#include "common/rng.glsl"

// from NRDsample
uint Bayer4x4 (uvec2 coords, uint frame) {
    uvec2 samplePosWrap = coords & uint(3);
    uint a = uint(2068378560) * uint( 1 - ( samplePosWrap.x >> 1 ) ) + uint(1500172770) * ( samplePosWrap.x >> 1 );
    uint b = uint( samplePosWrap.y + ( ( samplePosWrap.x & uint(1)) << 2 ) ) << 2;

    return ( ( a >> b ) + frame ) & uint(0xF);
}

float BayerNoise (vec2 pixel_coords, uint frame, inout uint rngState) {
    uvec2 coords = uvec2(floor(pixel_coords));

    uint bayer = Bayer4x4(coords, frame);
    float rng_float = RandomFloat(rngState);

    return (float(bayer) + rng_float) / 16.f;
}

#define FILE_BAYER
#endif