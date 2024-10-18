#version 430 core
#pragma import_defines(SHADING_MODEL, ALPHA_MODE)

#define SHADING_MODEL_UNLIT 0
#define SHADING_MODEL_STANDARD 1

#define ALPHA_MODE_OPAQUE 0
#define ALPHA_MODE_MASK 1
#define ALPHA_MODE_BLEND 2

#ifndef SHADING_MODEL
    #define SHADING_MODEL SHADING_MODEL_UNLIT
#endif

#ifndef ALPHA_MODE
    #define ALPHA_MODE ALPHA_MODE_OPAQUE
#endif

#define RENDERING_PATH_DEFERRED 0
#define RENDERING_PATH_FORWARD 1

#if (ALPHA_MODE != ALPHA_MODE_BLEND)
    #define RENDERING_PATH RENDERING_PATH_DEFERRED
#else
    #define RENDERING_PATH RENDERING_PATH_FORWARD
#endif

struct MaterialInputs
{
    vec3 fragPosVS;
    vec3 normalWS;
    vec3 tangentWS;
    vec3 bitangentWS;
    vec4 color;
    vec4 texcoord0;
    vec4 texcoord1;
};

struct MaterialOutputs
{
    vec3 emissive;
    float opaque;
    vec3 baseColor;
    float metallic;
    float roughness;
    float specular;
    vec3 normalVS;
    float occlusion;
};

struct PixelCommonData
{
    vec2 screenCoord;
    float fragDepth;
    vec3 fragPosVS;
    vec3 viewDirVS;
    vec3 reflDirVS;
    float NdV;
    vec3 F0;
};

void calcPixelCommonData(in MaterialOutputs mi, in MaterialOutputs mo, out PixelCommonData pcd)
{
    // forward
    pcd.screenCoord = gl_FragCoord.xy * uResolution.zw;
    pcd.fragDepth = gl_FragCoord.z;
    pcd.fragPosVS = v2f.fragPosVS;
    pcd.viewDirVS = normalize(-pcd.fragPosVS);
    // deferred
    pcd.screenCoord = uv;
    pcd.fragDepth = texelFetch(uSceneDepthTexture, iTexcoord, 0).r;
    vec4 fragPosNdc = vec4(vec3(pcd.screenCoord, pcd.fragDepth) * 2.0 - 1.0, 1.0);
    vec4 fragPosVS = uInverseProjectionMatrix * fragPosNdc;
    pcd.fragPosVS = fragPosVS.xyz * (1.0 / fragPosVS.w);
    pcd.viewDirVS = nromalize(-pcd.fragPosVS);
    //pcd.viewDirWS = mat3(uInverseViewMatrix) * viewDirVS;
    pcd.F0 = mix(vec3(0.04), mo.baseColor, mo.metallic);
}