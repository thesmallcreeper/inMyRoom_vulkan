#ifndef FILE_TONE_MAP

float ExpCompress(float val) {
    return 1.f - exp(-val);
}

float RangeCompress(float val, float threshold) {
    float v1 = val;
    float v2 = threshold + (1.f - threshold)*ExpCompress((val - threshold) / (1.f - threshold));
    return val < threshold ? v1 : v2;
}

vec3 ToneMap(vec3 in_color, float threshold) {
    float maxColor = max(in_color.x, max(in_color.y, in_color.z));
    float mappedMax = RangeCompress(maxColor, threshold);
    vec3 color_compressed = in_color * (mappedMax / maxColor);

    return color_compressed;
}

#define FILE_TONE_MAP
#endif