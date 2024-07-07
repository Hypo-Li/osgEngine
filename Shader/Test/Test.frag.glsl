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

uniform sampler2D uTexture0;
uniform sampler2D uTexture1;
uniform sampler2D uTexture2;
uniform sampler2D uTexture3;
uniform vec4 uColor2;
uniform vec4 uColor3;

float decodeColor(float data);
vec2 decodeColor(vec2 data);
vec3 decodeColor(vec3 data);
vec4 decodeColor(vec4 data);

float encodeColor(float color);
vec2 encodeColor(vec2 color);
vec3 encodeColor(vec3 color);
vec4 encodeColor(vec4 color);

vec3 decodeNormal(vec3 data);
vec3 encodeNormal(vec3 normal);

void calcMaterial(in MaterialInputs mi, out MaterialOutputs mo)
{
    vec4 tColor0 = decodeColor(texture(uTexture0, mi.texcoord0));
    vec4 tColor1 = texture(uTexture1, mi.texcoord0);
    vec3 local0 = mix(0.5, 1.0, tColor1.r) * mix(uColor3.rgb, uColor2.rgb, vec3(tColor1.b));
    mo.baseColor = encodeColor(tColor0.rgb * local0);

    vec4 tColor2 = texture(uTexture2, mi.texcoord0 * 0.05);
    float local1 = mix(-20.0, 20.0, tColor2.r);
    float local2 = clamp((local1 + (-mi.fragPosVS.z) - 10.0) / 10.0, 0.0, 1.0);
    float local3 = mix(tColor0.a, 0.5, local2);
    mo.roughness = mix(0.5, 0.2, local3);

    mo.normal = decodeNormal(texture(uTexture3, mi.texcoord0).rgb);
}

void main()
{
    MaterialInputs mi;
    mi.fragPosVS = v2f.fragPosVS;
    mi.normalWS = normalize(v2f.normalWS);
    mi.tangentWS = normalize(v2f.tangentWS);
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
    // GBuffer outputs
    fragData[0] = vec4(mo.emissive, 1.0);
    fragData[1] = vec4(mo.normal, 1.0);
    fragData[2] = vec4(mo.baseColor, mo.occlusion);
    fragData[3] = vec4(mo.metallic, mo.roughness, 0.0, 1.0);
#else
    // Forward outputs
    fragData = vec4(0.0);
#endif
}