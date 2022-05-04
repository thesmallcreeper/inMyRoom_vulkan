#ifndef FILE_RESERVOIR

#include "common/rng.glsl"

struct Reservoir {
    float choosen_weight;
    float weights_sum;
};

Reservoir CreateReservoir() {
    Reservoir return_reservoir;
    return_reservoir.choosen_weight = 0.f;
    return_reservoir.weights_sum = 0.f;

    return return_reservoir;
}

bool UpdateReservoir(float weight,
                     inout Reservoir reservoir,
                     inout uint rngState)
{
    reservoir.weights_sum += weight;

    float chance = weight/reservoir.weights_sum;
    float rng_float = RandomFloat(rngState);
    bool should_swap = bool(rng_float < chance);

    if (should_swap) {
        reservoir.choosen_weight = weight;
    }
    return should_swap;
}

#define FILE_RESERVOIR
#endif