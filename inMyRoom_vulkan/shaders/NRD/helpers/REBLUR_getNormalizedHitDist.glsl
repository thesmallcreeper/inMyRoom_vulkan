#ifndef FILE_GET_NORMALIZED_HIT_DIST

#ifndef HIT_DIST_PARAMS_X
#define HIT_DIST_PARAMS_X 3.0f
#endif

#ifndef HIT_DIST_PARAMS_Y
#define HIT_DIST_PARAMS_Y 0.1f
#endif

#ifndef HIT_DIST_PARAMS_Z
#define HIT_DIST_PARAMS_Z 20.0f
#endif

#ifndef HIT_DIST_PARAMS_W
#define HIT_DIST_PARAMS_W -25.0f
#endif

float _REBLUR_GetHitDistanceNormalization( float viewZ, float roughness ) {
    return ( HIT_DIST_PARAMS_X + abs( viewZ ) * HIT_DIST_PARAMS_Y ) * mix( 1.0, HIT_DIST_PARAMS_Z, clamp( exp2( HIT_DIST_PARAMS_W * roughness * roughness ), 0.f, 1.f ) );
}

float REBLUR_FrontEnd_GetNormHitDist( float hitDist, float viewZ, float roughness ) {
    float f = _REBLUR_GetHitDistanceNormalization( viewZ, roughness );

    return clamp( hitDist / f, 0.f, 1.f );
}

float REBLUR_FrontEnd_GetNormHitDist( float hitDist, float viewZ) {
    return REBLUR_FrontEnd_GetNormHitDist( hitDist, viewZ, 1.f);
}

float REBLUR_GetHitDist( float normHitDist, float viewZ, float roughness ) {
    float scale = _REBLUR_GetHitDistanceNormalization( viewZ, roughness );

    return normHitDist * scale;
}

float REBLUR_GetHitDist( float normHitDist, float viewZ) {
    return REBLUR_GetHitDist( normHitDist, viewZ, 1.f);
}

#define FILE_GET_NORMALIZED_HIT_DIST
#endif