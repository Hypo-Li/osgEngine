#version 430 core
// TODO: add ALPHA_MASK_THRESHOLD
#pragma import_defines(SHADING_MODEL, ALPHA_MODE, SHADOW_CAST)

#ifndef SHADOW_CAST
#define SHADOW_CAST 0
#endif

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

in V2F
{
    vec3 fragPosVS;
    vec3 normalVS;
    vec4 tangentVS;
    vec4 color;
    vec4 texcoord0;
    vec4 texcoord1;
    vec4 fragPosCS;
    vec4 prevFragPosCS;
} v2f;

#if (RENDERING_PATH == RENDERING_PATH_DEFERRED)
    #if (SHADING_MODEL == SHADING_MODEL_UNLIT)
out vec4 fragData[2];
    #else
out vec4 fragData[6];
    #endif
#else
    #if (SHADING_MODEL == SHADING_MODEL_UNLIT)
out vec4 fragData[2];
    #else
out vec4 fragData;
    #endif
#endif

#extension GL_GOOGLE_include_directive : enable
#include "../Lighting/Common.glsl"

uniform vec4 uResolution;

layout(std140, binding = 0) uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uInverseViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uInverseProjectionMatrix;
    mat4 uReprojectionMatrix;
    vec2 uJitterPixels;
    vec2 uPrevJitterPixels;
};

struct MaterialInputs
{
    vec3 fragPosVS;
    vec3 normalVS;
    vec3 normalWS;
    vec3 tangentVS;
    vec3 tangentWS;
    vec4 color;
    vec4 texcoord0;
    vec4 texcoord1;
    vec3 viewDirWS;
    vec3 reflDirWS;
};

struct MaterialOutputs
{
    vec3 emissive;
    float opacity;
    vec3 baseColor;
    float metallic;
    float roughness;
    float specular;
    vec3 normal;
    float occlusion;
};

void calcPixelParameters(in MaterialOutputs mo, out PixelParameters pp)
{
    pp.shadingModel = uint(SHADING_MODEL * 255.0);
    pp.baseColor = pow(mo.baseColor, vec3(2.2)) * mo.opacity;
    pp.metallic = mo.metallic;
    pp.roughness = max(mo.roughness, 0.05);
    float a = pp.roughness * pp.roughness;
    float a2 = a * a;
    pp.specular = mo.specular;
    pp.normalWS = normalize(mo.normal);
    pp.occlusion = mo.occlusion;
    pp.fragPosVS = v2f.fragPosVS;
    pp.viewDirWS = mat3(uInverseViewMatrix) * normalize(-pp.fragPosVS);
    pp.NoV = max(dot(pp.normalWS, pp.viewDirWS), 0.0);
    pp.reflDirWS = reflect(-pp.viewDirWS, pp.normalWS);
    pp.reflDirWS = mix(pp.reflDirWS, pp.normalWS, a2);
    pp.F0 = mix(vec3(pp.specular * 0.08), pp.baseColor, pp.metallic);
}

void calcMaterial(in MaterialInputs mi, inout MaterialOutputs mo);

void initMaterialInputs(inout MaterialInputs mi)
{
    mat3 inverseViewMatrix3 = mat3(uInverseViewMatrix);
    mi.fragPosVS = v2f.fragPosVS;
    mi.normalVS = normalize(v2f.normalVS);
    mi.normalWS = inverseViewMatrix3 * mi.normalVS;
    mi.tangentVS = normalize(v2f.tangentVS.xyz);
    mi.tangentWS = inverseViewMatrix3 * mi.tangentVS;
    mi.color = v2f.color;
    mi.texcoord0 = v2f.texcoord0;
    mi.texcoord1 = v2f.texcoord1;
    mi.viewDirWS = inverseViewMatrix3 * normalize(-v2f.fragPosVS);
    mi.reflDirWS = reflect(-mi.viewDirWS, mi.normalWS);
}

void initMaterialOutputs(inout MaterialOutputs mo)
{
    mo.emissive = vec3(0);
    mo.opacity = 1;
    mo.baseColor = vec3(0.8);
    mo.metallic = 0;
    mo.roughness = 0.5;
    mo.specular = 0.5;
    mo.normal = vec3(0, 0, 1);
    mo.occlusion = 1;
}

// 计算世界空间法线
void calcNormalWS(in MaterialInputs mi, inout MaterialOutputs mo)
{
    vec3 bitangentVS = normalize(cross(mi.normalVS, mi.tangentVS)) * v2f.tangentVS.w;
    mo.normal = mat3(uInverseViewMatrix) * mat3(mi.tangentVS, bitangentVS, mi.normalVS) * mo.normal;
}

vec3 calcMotionVector()
{
    vec2 screenPos = v2f.fragPosCS.xy / v2f.fragPosCS.w + uJitterPixels; 
    vec2 prevScreenPos = v2f.prevFragPosCS.xy / v2f.prevFragPosCS.w + uPrevJitterPixels;
    float deviceZ = v2f.fragPosCS.z / v2f.fragPosCS.w; 
    float prevDeviceZ = v2f.prevFragPosCS.z / v2f.prevFragPosCS.w; 
    vec3 motionVector = vec3(screenPos - prevScreenPos, deviceZ - prevDeviceZ); 
    return motionVector * 0.5; 
}

void main()
{
    MaterialInputs mi;
    initMaterialInputs(mi);
    MaterialOutputs mo;
    initMaterialOutputs(mo);
    calcMaterial(mi, mo);

#if (ALPHA_MODE == ALPHA_MODE_MASK)
    if (mo.opacity < 0.5)
        discard;
#endif

    calcNormalWS(mi, mo);

#if (RENDERING_PATH == RENDERING_PATH_DEFERRED)
    #if (SHADING_MODEL == SHADING_MODEL_UNLIT)
    fragData[0] = vec4(mo.baseColor, 1.0);
    fragData[1] = vec4(1);
    #else
    fragData[0] = vec4(mo.emissive, 1.0);
    fragData[1] = vec4(mo.normal * 0.5 + 0.5, 1.0);
    fragData[2] = vec4(mo.metallic, mo.roughness, mo.specular, SHADING_MODEL / 255.0);
    fragData[3] = vec4(mo.baseColor, mo.occlusion);
    fragData[4] = vec4(0.0, 0.0, 0.0, 0.0);
    fragData[5] = vec4(calcMotionVector(), 0.0);
    #endif
#else
    #if (SHADING_MODEL == SHADING_MODEL_UNLIT)
    fragData[0] = vec4(mo.baseColor, mo.opacity);
    fragData[1] = vec4(1);
    #else
    PixelParameters pp;
    calcPixelParameters(mo, pp);
    fragData = vec4(evaluateLighs(pp), mo.opacity);
    #endif
#endif
}
