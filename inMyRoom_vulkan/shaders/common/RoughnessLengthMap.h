#ifndef FILE_ROUGHNESS_LENGTH_MAP

#include "common/defines.h"
#include "common/structs/cppInterface.h"

float RoughnessToLength(float roughness)
{
    FLOAT_T roughPow2 = roughness * roughness;
    FLOAT_T roughPow3 = roughPow2 * roughness;
    FLOAT_T roughPow4 = roughPow2 * roughPow2;
    VEC4 roughVec = VEC4_C(roughPow4, roughPow3, roughPow2, roughness);

    VEC4 P;
    VEC4 Q;
    if (roughness < BP_R_TO_L) {
        P = VEC4_C(COEFF_R_TO_L_LP1, COEFF_R_TO_L_LP2, COEFF_R_TO_L_LP3, COEFF_R_TO_L_LP4);
        Q = VEC4_C(COEFF_R_TO_L_LQ1, COEFF_R_TO_L_LQ2, COEFF_R_TO_L_LQ3, COEFF_R_TO_L_LQ4);
    } else {
        P = VEC4_C(COEFF_R_TO_L_RP1, COEFF_R_TO_L_RP2, COEFF_R_TO_L_RP3, COEFF_R_TO_L_RP4);
        Q = VEC4_C(COEFF_R_TO_L_RQ1, COEFF_R_TO_L_RQ2, COEFF_R_TO_L_RQ3, COEFF_R_TO_L_RQ4);
    }

    FLOAT_T result = DOT(roughVec, P) / DOT(roughVec, Q);
    result = MIN(result, TEX_FILTERING_MAX_LENGTH);
    return result;
}

float LengthToRoughness(float length)
{
    length = MAX(length, TEX_FILTERING_MIN_LENGTH);

    FLOAT_T lengthPow2 = length * length;
    FLOAT_T lengthPow3 = lengthPow2 * length;
    FLOAT_T lengthPow4 = lengthPow2 * lengthPow2;
    VEC4 lengthVec = VEC4_C(lengthPow4, lengthPow3, lengthPow2, length);

    FLOAT_T A, B;
    VEC4 P;
    VEC4 Q;
    if (length < BP_L_TO_R) {
        A = COEFF_L_TO_R_LA;
        B = COEFF_L_TO_R_LB;
        P = VEC4_C(COEFF_L_TO_R_LP1, COEFF_L_TO_R_LP2, COEFF_L_TO_R_LP3, COEFF_L_TO_R_LP4);
        Q = VEC4_C(COEFF_L_TO_R_LQ1, COEFF_L_TO_R_LQ2, COEFF_L_TO_R_LQ3, COEFF_L_TO_R_LQ4);
    } else {
        A = COEFF_L_TO_R_RA;
        B = COEFF_L_TO_R_RB;
        P = VEC4_C(COEFF_L_TO_R_RP1, COEFF_L_TO_R_RP2, COEFF_L_TO_R_RP3, COEFF_L_TO_R_RP4);
        Q = VEC4_C(COEFF_L_TO_R_RQ1, COEFF_L_TO_R_RQ2, COEFF_L_TO_R_RQ3, COEFF_L_TO_R_RQ4);
    }

    FLOAT_T result = DOT(lengthVec, P) / DOT(lengthVec, Q) + A*SQRT(B*(1.f - length));
    result = MAX(result, TEX_FILTERING_MIN_ROUGH);
    result = MIN(result, TEX_FILTERING_MAX_ROUGH);
    return result;
}

#endif