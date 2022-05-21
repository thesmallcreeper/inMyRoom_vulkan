#ifndef FILE_ENVIRONMENT_TERM

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// Ross BRDF estimation as provided from NRD
vec3 EnvironmentTerm(vec3 f0, float NoV, float a)
{
    float f = pow( 1.0 - NoV, 5.0 * exp( -2.69 * a ) ) / ( 1.0 + 22.7 * pow( a, 1.5 ) );

    float scale = 1.0 - f;
    float bias = f;

    return clamp( f0 * scale + vec3(bias), 0.f, 1.f);
}

#define FILE_ENVIRONMENT_TERM
#endif