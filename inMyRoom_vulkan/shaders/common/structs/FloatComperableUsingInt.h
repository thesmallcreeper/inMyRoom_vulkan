#ifndef FILE_FLOAT_COMPARABLE_USING_INT

#include "common/structs/cppInterface.h"

#ifdef ENABLE_CPP_INTERFACE
#include <bit>
int floatBitsToInt(float data) {
    return std::bit_cast<int>(data);
}

float intBitsToFloat(int data) {
    return std::bit_cast<float>(data);
}
#endif

int FloatComparableUsingInt(float data) {
    int data_as_int = floatBitsToInt(data);
    if (data_as_int < 0) {
        data_as_int = data_as_int ^ 0x7FFFFFFF;
    }

    return data_as_int;
}


float FloatComparableUsingInt(int data) {
    if (data < 0) {
        data = data ^ 0x7FFFFFFF;
    }
    float data_as_float = intBitsToFloat(data);

    return data_as_float;
}

#define FILE_FLOAT_COMPARABLE_USING_INT
#endif
