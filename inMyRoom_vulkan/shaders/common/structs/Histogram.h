#ifndef FILE_HISTOGRAM

#include "common/structs/cppInterface.h"

#define HISTOGRAM_BUCKETS_COUNT 64
struct Histogram {
    FLOAT_T buckets[HISTOGRAM_BUCKETS_COUNT];
};

#define FILE_HISTOGRAM
#endif