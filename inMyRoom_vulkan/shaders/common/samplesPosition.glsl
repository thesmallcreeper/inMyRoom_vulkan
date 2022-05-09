#ifndef FILE_SAMPLES_POSITIONS

vec2 InputSamplesPositions(int i) {
    #if !defined(MULTISAMPLED_INPUT)
        return vec2(0.5f, 0.5f);
    #elif MULTISAMPLED_INPUT == 2
        switch (i) {
            case 0:
                return vec2(0.75f, 0.75f);
            case 1:
                return vec2(0.25f, 0.25f);
        }
    #elif MULTISAMPLED_INPUT == 4
        switch (i) {
            case 0:
                return vec2(0.375f, 0.125f);
            case 1:
                return vec2(0.875f, 0.375f);
            case 2:
                return vec2(0.125f, 0.625f);
            case 3:
                return vec2(0.625f, 0.875f);
        }
    #elif MULTISAMPLED_INPUT == 8
        switch (i) {
            case 0:
                return vec2(0.5625, 0.3125);
            case 1:
                return vec2(0.4375, 0.6875);
            case 2:
                return vec2(0.8125, 0.5625);
            case 3:
                return vec2(0.3125, 0.1875);
            case 4:
                return vec2(0.1875, 0.8125);
            case 5:
                return vec2(0.0625, 0.4375);
            case 6:
                return vec2(0.6875, 0.9375);
            case 7:
                return vec2(0.9375, 0.0625);
        }
    #elif MULTISAMPLED_INPUT == 16
        switch (i) {
            case 0:
                return vec2(0.5625, 0.5625);
            case 1:
                return vec2(0.4375, 0.3125);
            case 2:
                return vec2(0.3125, 0.625);
            case 3:
                return vec2(0.75, 0.4375);
            case 4:
                return vec2(0.1875, 0.375);
            case 5:
                return vec2(0.625, 0.8125);
            case 6:
                return vec2(0.8125, 0.6875);
            case 7:
                return vec2(0.6875, 0.1875);
            case 8:
                return vec2(0.375, 0.875);
            case 9:
                return vec2(0.5, 0.0625);
            case 10:
                return vec2(0.25, 0.125);
            case 11:
                return vec2(0.125, 0.75);
            case 12:
                return vec2(0.0, 0.5);
            case 13:
                return vec2(0.9375, 0.25);
            case 14:
                return vec2(0.875, 0.9375);
            case 15:
                return vec2(0.0625, 0.0);
        }
    #endif
}


#define FILE_SAMPLES_POSITIONS
#endif