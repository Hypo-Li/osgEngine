#version 430 core
// TODO: add ALPHA_MASK_THRESHOLD
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

uniform mat4 osg_ViewMatrixInverse;

in V2F
{
    vec3 fragPosVS;
    vec3 normalVS;
    vec4 tangentVS;
    vec4 color;
    vec4 texcoord0;
    vec4 texcoord1;
} v2f;

#if (RENDERING_PATH == RENDERING_PATH_DEFERRED)
out vec4 fragData[5];
#else
out vec4 fragData;
#endif

struct MaterialInputs
{
    vec3 fragPosVS;
    vec3 normalVS;
    vec3 tangentVS;
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
    vec3 normal;
    float occlusion;
};

struct PixelCommonData
{
    vec3 viewDirWS;
    vec3 reflDirWS;
    float NdV;
    vec3 F0;

};

struct DirectionalLight
{
    vec3 direction;
    vec3 intensity;
};

void calcPixelCommonData(in MaterialOutputs mo, out PixelCommonData pcd)
{
    //vec3 viewDirVS = normalize(-v2f.fragPosVS);
    //pcd.viewDirWS = mat3(uInverseViewMatrix) * viewDirVS;
    pcd.F0 = mix(vec3(0.04), mo.baseColor, mo.metallic);
}

void calcMaterial(in MaterialInputs mi, inout MaterialOutputs mo);

void evaluateDirectionalLight(in MaterialOutputs mo, in PixelCommonData pcd, inout vec3 color)
{

}

void evaluateIBL(in MaterialOutputs mo, in PixelCommonData pcd, inout vec3 color)
{

}

vec3 evaluateLighting(in const MaterialOutputs mo)
{
#if ((RENDERING_PATH == RENDERING_PATH_FORWARD) && (SHADING_MODEL == SHADING_MODEL_UNLIT))
    return mo.emissive;
#else
    PixelCommonData pcd;
    calcPixelCommonData(mo, pcd);
    vec3 color = vec3(0.0);
    evaluateDirectionalLight(mo, pcd, color);
    evaluateIBL(mo, pcd, color);
    return color;
#endif
}

void initMaterialInputs(inout MaterialInputs mi)
{
    mi.fragPosVS = v2f.fragPosVS;
    mi.normalVS = normalize(v2f.normalVS);
    mi.tangentVS = normalize(v2f.tangentVS.xyz);
    mi.color = v2f.color;
    mi.texcoord0 = v2f.texcoord0;
    mi.texcoord1 = v2f.texcoord1;
}

void initMaterialOutputs(inout MaterialOutputs mo)
{
    mo.emissive = vec3(0);
    mo.opaque = 1;
    mo.baseColor = vec3(0.8);
    mo.metallic = 0;
    mo.roughness = 0.5;
    mo.specular = 0.5;
    mo.normal = vec3(0, 0, 1);
    mo.occlusion = 1;
}

void calcNormal(in MaterialInputs mi, inout MaterialOutputs mo)
{
    vec3 bitangentVS = normalize(cross(mi.normalVS, mi.tangentVS)) * v2f.tangentVS.w;
    mo.normal = mat3(osg_ViewMatrixInverse) * mat3(mi.tangentVS, bitangentVS, mi.normalVS) * mo.normal;
}

void main()
{
    MaterialInputs mi;
    initMaterialInputs(mi);
    MaterialOutputs mo;
    initMaterialOutputs(mo);
    calcMaterial(mi, mo);

#if (ALPHA_MODE == ALPHA_MODE_MASK)
    if (mo.opaque < 0.5)
        discard;
#endif

    calcNormal(mi, mo);

#if (RENDERING_PATH == RENDERING_PATH_DEFERRED)
    fragData[0] = vec4(mo.emissive, 1.0);
    fragData[1] = vec4(mo.normal * 0.5 + 0.5, 1.0);
    fragData[2] = vec4(mo.metallic, mo.roughness, mo.specular, SHADING_MODEL / 255.0);
    fragData[3] = vec4(mo.baseColor, mo.occlusion);
    fragData[4] = vec4(0);
#else
    fragData = vec4(/*evaluateLighting(mo)*/mo.baseColor, mo.opaque);
#endif
}
