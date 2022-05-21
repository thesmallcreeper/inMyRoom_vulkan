#ifndef FILE_NRD_PACK_NORMAL_ROUGHNESS

// https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
// and NRD library
vec2 OctWrap(vec2 v) {
    return ( 1.0 - abs( v.yx ) ) * ( step( 0.0, v.xy ) * 2.f - 1.f );
}

vec2 EncodeUnitVector(vec3 v) {
    v /= ( abs( v.x ) + abs( v.y ) + abs( v.z ) );
    v.xy = v.z >= 0.0 ? v.xy : OctWrap( v.xy );
    v.xy = v.xy * 0.5 + 0.5;
    return v.xy;
}

vec3 DecodeUnitVector(vec2 f) {
    f = f * 2.0 - 1.0;

    // https://twitter.com/Stubbesaurus/status/937994790553227264
    vec3 n = vec3( f.x, f.y, 1.0 - abs( f.x ) - abs( f.y ) );
    float t = clamp( -n.z, 0.f, 1.f );
    n.xy += (step( 0.0, n.xy ) * 2.0 - 1.0) * (-t);
    return normalize( n );
}

vec4 NRD_FrontEnd_PackNormalRoughness( vec3 N, float roughness, uint materialID ) {
    vec4 p;
    p.xy = EncodeUnitVector( N );
    p.z = roughness;
    p.w = clamp( ( materialID + 0.5 ) / 3.0, 0.f, 1.f);

    return p;
}

vec4 NRD_FrontEnd_PackNormalRoughness( vec3 N, float roughness ) {
    return NRD_FrontEnd_PackNormalRoughness( N, roughness, 0 );
}

struct NRDunpackNormalRoughness {
    vec3 normal;
    float roughness;
    uint materialID;
};

NRDunpackNormalRoughness NRD_FrontEnd_UnpackNormalRoughness( vec4 p )
{
    NRDunpackNormalRoughness return_struct;
    return_struct.normal = DecodeUnitVector( p.xy );
    return_struct.roughness = p.z;
    return_struct.materialID = uint( round(p.w * 3.0) );

    return return_struct;
}

#define FILE_NRD_PACK_NORMAL_ROUGHNESS
#endif