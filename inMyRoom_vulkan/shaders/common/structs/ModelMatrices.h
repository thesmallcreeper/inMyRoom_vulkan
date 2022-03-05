#ifndef MODEL_MATRICES

#include "common/structs/cppInterface.h"

struct ModelMatrices {
    MAT4_T positionMatrix;
    MAT4_T normalMatrix;
};

#define MODEL_MATRICES
#endif