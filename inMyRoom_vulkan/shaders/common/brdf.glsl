#ifndef FILE_BRDF

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

float GGXdistribution(float a, float NdotH) {
    float NdotH_squared = NdotH * NdotH;
    float a_squared = a * a;
    float divisor = NdotH_squared * (a_squared - 1.f) + 1.f;
    float D_sqrt = (a / divisor);
    float D = D_sqrt * D_sqrt / M_PI;

    return D;
}

// Smith Joint GGX
float SmithsVisibility(vec3 eye, vec3 light, vec3 normal, float a) {
    float a_squared = a * a;

    float NdotL = dot(normal, light);
    float NdotV = dot(normal, eye);

    float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - a_squared) + a_squared);
    float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - a_squared) + a_squared);

    float GGX = GGXV + GGXL;
    if (GGX > 0.0)
    {
        return 0.5 / GGX;
    }
    return 0.0;
}

vec3 BRDF(vec3 baseColor, float a, float metallic, vec3 eye, vec3 light, vec3 normal) {

    vec3 halfvector = normalize(eye + light);
    float NdotH = dot(normal, halfvector);
    float HdotL = dot(halfvector, light);

    vec3 c_diff = mix(baseColor.rgb, vec3(0.f), metallic);
    vec3 f0 = mix(vec3(0.04f), baseColor.rgb, metallic);

    vec3 F = f0 + (vec3(1.f) - f0) * pow(1.f - max(HdotL, 0.f), 5.f);

    vec3 f_diffuse = (vec3(1.f) - F) * (1.f / M_PI) * c_diff;
    vec3 f_specular = F * GGXdistribution(a, NdotH) * SmithsVisibility(eye, light, normal, a);

    return f_diffuse + f_specular;
}

#define FILE_BRDF
#endif