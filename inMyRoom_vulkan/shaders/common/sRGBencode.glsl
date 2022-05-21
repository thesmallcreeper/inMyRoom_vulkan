#ifndef FILE_SRGB_ENCODE

float sRGBencode(float linear) {
    linear = clamp(linear, 0.f, 1.f);

    float sRGB = 0.f;
    if (linear < 0.0031308f)
        sRGB = linear * 12.92f;
    else
        sRGB =  pow(linear, 1.0f / 2.4f) * 1.055f - 0.055f;

    return sRGB;
}

vec4 sRGBencode(vec4 linear) {
    return vec4(sRGBencode(linear.r),
                sRGBencode(linear.g),
                sRGBencode(linear.b),
                linear.a);
}

float sRGBdecode(float sRGB) {
    sRGB = clamp(sRGB, 0.f, 1.f);

    float linear = 0.f;
    if(sRGB < 0.04045f)
    linear = sRGB / 12.92f;
    else
    linear = pow((sRGB + 0.055f) / 1.055f, 2.4f);

    return linear;
}

vec4 sRGBdecode(vec4 sRGB) {
    return vec4(sRGBdecode(sRGB.r),
                sRGBdecode(sRGB.g),
                sRGBdecode(sRGB.b),
                sRGB.a);
}

#define FILE_SRGB_ENCODE
#endif