#ifndef FILE_LUMINANCE

// Rec. 601
float Luminance(vec3 rgb_color) {
    float luminance = dot(rgb_color, vec3(0.2990, 0.5870, 0.1140));
    return luminance;
}

#define FILE_LUMINANCE
#endif