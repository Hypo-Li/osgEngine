#version 460 core
in V2F
{
    vec3 fragPosVS;
    vec3 normalWS;
    vec4 tangentWS;
    vec4 color;
    vec2 texcoord0;
    vec2 texcoord1;
} v2f;

out vec4 fragData[4];

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
    vec3 baseColor;
    float metallic;
    float roughness;
    vec3 normal;
    vec3 emissive;
    float occlusion;
};

void calcMaterial(in MaterialInputs mi, out MaterialOutputs mo);

void main()
{
    MaterialInputs mi;
    mi.fragPosVS = v2f.fragPosVS;
    mi.normalWS = v2f.normalWS;
    mi.tangentWS = v2f.tangentWS;
    mi.color = v2f.color;
    mi.texcoord0 = v2f.texcoord0;
    mi.texcoord1 = v2f.texcoord1;
    MaterialOutputs mo;
    calcMaterial(mi, mo);
    fragData[0] = vec4(mo.emissive, 1.0);
    fragData[1] = vec4(mo.normal, 1.0);
    fragData[2] = vec4(mo.baseColor, mo.occlusion);
    fragData[3] = vec4(mo.metallic, mo.roughness, 0.0, 1.0);
}