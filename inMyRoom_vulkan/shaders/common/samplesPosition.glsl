#ifndef FILE_SAMPLES_POSITIONS

#ifdef MULTISAMPLED_INPUT
#define SAMPLES_COUNT MULTISAMPLED_INPUT
#endif

#ifdef MORPHOLOGICAL_MSAA
#define SAMPLES_COUNT MORPHOLOGICAL_MSAA
#endif

vec2 InputSamplesPositions(int i) {
    #if !defined(SAMPLES_COUNT)
        return vec2(0.5f, 0.5f);
    #elif SAMPLES_COUNT == 2
        vec2 samples_pos[2] = {
            vec2(0.75f, 0.75f),
            vec2(0.25f, 0.25f)
        };

        return samples_pos[i];
    #elif SAMPLES_COUNT == 4
        vec2 samples_pos[4] = {
            vec2(0.375f, 0.125f),
            vec2(0.875f, 0.375f),
            vec2(0.125f, 0.625f),
            vec2(0.625f, 0.875f)
        };

        return samples_pos[i];
    #elif SAMPLES_COUNT == 8
        vec2 samples_pos[8] = {
            vec2(0.5625, 0.3125),
            vec2(0.4375, 0.6875),
            vec2(0.8125, 0.5625),
            vec2(0.3125, 0.1875),
            vec2(0.1875, 0.8125),
            vec2(0.0625, 0.4375),
            vec2(0.6875, 0.9375),
            vec2(0.9375, 0.0625)
        };

        return samples_pos[i];
    #elif SAMPLES_COUNT == 16
        vec2 samples_pos[16] = {
            vec2(0.5625, 0.5625),
            vec2(0.4375, 0.3125),
            vec2(0.3125, 0.625),
            vec2(0.75, 0.4375),
            vec2(0.1875, 0.375),
            vec2(0.625, 0.8125),
            vec2(0.8125, 0.6875),
            vec2(0.6875, 0.1875),
            vec2(0.375, 0.875),
            vec2(0.5, 0.0625),
            vec2(0.25, 0.125),
            vec2(0.125, 0.75),
            vec2(0.0, 0.5),
            vec2(0.9375, 0.25),
            vec2(0.875, 0.9375),
            vec2(0.0625, 0.0)
        };

        return samples_pos[i];
    #endif
}


#define FILE_SAMPLES_POSITIONS
#endif