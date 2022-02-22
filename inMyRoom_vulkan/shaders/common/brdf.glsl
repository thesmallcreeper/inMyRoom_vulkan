
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

float GGXdistribution(float a, float NdotH) {
    float NdotH_squared = NdotH * NdotH;
    float a_squared = a * a;
    float divisor = (NdotH_squared * (a_squared - 1.f) + 1.f);
    float D = a_squared / (M_PI * divisor * divisor);
    return D;
}

float SmithsVisibility(vec3 eye, vec3 light, vec3 normal, float a) {
    float a_squared = a * a;

    float NdotL = dot(normal, light);
    float NdotL_abs = abs(NdotL);
    float NdotL_squared = NdotL * NdotL;

    float NdotV = dot(normal, eye);
    float NdotV_abs = abs(NdotV);
    float NdotV_squared = NdotV * NdotV;

    float divisor_1 = NdotL_abs + sqrt(a_squared + (1.f - a_squared) * NdotL_squared);
    float divisor_2 = NdotV_abs + sqrt(a_squared + (1.f - a_squared) * NdotV_squared);

    return divisor_1 * divisor_2;
}

vec3 BRDF(vec3 baseColor, float roughness, float metallic, vec3 eye, vec3 light, vec3 normal) {
    float a = roughness * roughness;

    vec3 halfvector = normalize(eye + light);
    float NdotH = dot(normal, halfvector);
    float HdotL = dot(halfvector, light);

    vec3 c_diff = mix(baseColor.rgb, vec3(0.f), metallic);
    vec3 f0 = mix(vec3(0.04f), baseColor.rgb, metallic);

    vec3 F = f0 + (vec3(1.f) - f0) * pow(1.f - max(HdotL, 0.f), 5);

    vec3 f_diffuse = (vec3(1.f) - F) * (1.f / M_PI) * c_diff;
    vec3 f_specular = F * GGXdistribution(a, NdotH) * SmithsVisibility(eye, light, normal, a);

    return f_diffuse + f_specular;
}