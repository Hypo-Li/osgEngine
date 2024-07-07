#version 460 core
#pragma import_defines(SHADING_MODEL)
#pragma import_defines(ALPHA_MODE)

#ifdef SHADING_MODEL
#define UNLIT 0
#define STANDARD 1
#endif

#ifdef ALPHA_MODE
#define OPAQUE 0
#define ALPHA_MASK 1
#define ALPHA_BLEND 2
#endif

in V2F
{
    vec3 fragPosVS;
    vec3 normalWS;
    vec4 tangentWS;
    vec4 color;
    vec2 texcoord0;
    vec2 texcoord1;
} v2f;

#if (ALPHA_MODE != ALPHA_BLEND)
out vec4 fragData[4];
#else
out vec4 fragData;
#endif

struct MaterialInputs
{
    vec3 fragPosVS;
    vec3 normalWS;
    vec4 tangentWS;
    vec4 color;
    vec2 texcoord0;
    vec2 texcoord1;
};

struct MaterialOutputs
{
    vec3 emissive;
    float opaque;
#if (SHADING_MODEL >= STANDARD)
    vec3 baseColor;
    float metallic;
    float roughness;
    vec3 normal;
    float occlusion;
#endif
};

float decodeColor(float data) { return pow(data, 2.2); }
vec2 decodeColor(vec2 data) { return pow(data, vec2(2.2)); }
vec3 decodeColor(vec3 data) { return pow(data, vec3(2.2)); }
vec4 decodeColor(vec4 data) { return pow(data, vec4(2.2)); }

float encodeColor(float color) { return pow(color, 0.454545); }
vec2 encodeColor(vec2 color) { return pow(color, vec2(0.454545)); }
vec3 encodeColor(vec3 color) { return pow(color, vec3(0.454545)); }
vec4 encodeColor(vec4 color) { return pow(color, vec4(0.454545)); }

vec3 decodeNormal(vec3 data) { return normalize(data * 2.0 - 1.0); }
vec3 encodeNormal(vec3 normal) { return normal * 0.5 + 0.5; }

void calcMaterial(in MaterialInputs mi, out MaterialOutputs mo);

void main()
{
    MaterialInputs mi;
    mi.fragPosVS = v2f.fragPosVS;
    mi.normalWS = normalize(v2f.normalWS);
    mi.tangentWS = vec4(normalize(v2f.tangentWS.xyz), v2f.tangentWS.w);
    mi.color = v2f.color;
    mi.texcoord0 = v2f.texcoord0;
    mi.texcoord1 = v2f.texcoord1;
    MaterialOutputs mo;
    calcMaterial(mi, mo);

#if (ALPHA_MODE == ALPHA_MASK)
    if (mo.opaque < 0.5)
        discard;
#endif

#if (ALPHA_MODE != ALPHA_BLEND)
    vec3 bitangentWS = normalize(cross(mi.tangentWS.xyz, mi.normalWS)) * mi.tangentWS.w;
    mat3 tbn = mat3(mi.tangentWS.xyz, bitangentWS, mi.normalWS);

    // GBuffer outputs
    fragData[0] = vec4(mo.emissive, 1.0);
    fragData[1] = vec4(encodeNormal(tbn * mo.normal), 1.0);
    fragData[2] = vec4(encodeColor(mo.baseColor), 1.0);
    fragData[3] = vec4(mo.metallic, mo.roughness, 0.0, 1.0);
#else
    // Forward outputs
    fragData = vec4(0.0);
#endif
}