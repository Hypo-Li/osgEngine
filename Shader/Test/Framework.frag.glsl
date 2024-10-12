#version 430 core
#pragma import_defines(SHADING_MODEL, ALPHA_MODE)

#ifdef SHADING_MODEL
#define SHADING_MODEL_UNLIT 0
#define SHADING_MODEL_STANDARD 1
#endif

#ifdef ALPHA_MODE
#define ALPHA_MODE_OPAQUE 0
#define ALPHA_MODE_MASK 1
#define ALPHA_MODE_BLEND 2
#endif

in V2F
{
    vec3 fragPosVS;
    vec3 normalWS;
    vec4 tangentWS;
    vec4 color;
    vec4 texcoord0;
    vec4 texcoord1;
} v2f;

#if ((SHADING_MODEL != SHADING_MODEL_UNLIT) && (ALPHA_MODE != ALPHA_MODE_BLEND))
out vec4 fragData[5];
#else
out vec4 fragData;
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
    vec3 normal;
    float occlusion;
};

void calcMaterial(in MaterialInputs mi, out MaterialOutputs mo);

void main()
{
    MaterialInputs mi;
    mi.fragPosVS = v2f.fragPosVS;
    mi.normalWS = normalize(v2f.normalWS);
    mi.tangentWS = normalize(v2f.tangentWS.xyz);
    mi.bitangentWS = normalize(cross(mi.normalWS, mi.tangentWS)) * v2f.tangentWS.w;
    mi.color = v2f.color;
    mi.texcoord0 = v2f.texcoord0;
    mi.texcoord1 = v2f.texcoord1;
    MaterialOutputs mo;
    calcMaterial(mi, mo);

#if (ALPHA_MODE == ALPHA_MODE_MASK)
    if (mo.opaque < 0.5)
        discard;
#endif

#if ((SHADING_MODEL != SHADING_MODEL_UNLIT) && (ALPHA_MODE != ALPHA_MODE_BLEND))
    fragData[0] = vec4(mo.emissive, 1.0);
    fragData[1] = vec4(mo.normal * 0.5 + 0.5, 1.0);
    fragData[2] = vec4(mo.metallic, mo.roughness, mo.specular, SHADING_MODEL / 255.0);
    fragData[3] = vec4(mo.baseColor, mo.occlusion);
    fragData[4] = vec4(0);
#else
    fragData = vec4(mo.emissive, mo.opaque);
#endif
}
