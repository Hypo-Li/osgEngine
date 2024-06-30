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

#define UNLIT 0
#define STANDARD 1
#define SHADING_MODEL STANDARD

#define OPAQUE 0
#define ALPHA_MASK 1
#define ALPHA_BLEND 2
#define ALPHA_MODE OPAQUE

struct MaterialInputParameters
{
    vec3 fragPosVS;
    vec3 normalWS;
    vec4 tangentWS;
    vec4 color;
    vec2 texcoord0;
    vec2 texcoord1;
};

struct MaterialOutputParameters
{
    vec3 baseColor;
    float metallic;
    float roughness;
    vec3 normal;
    vec3 emissive;
    float occlusion;
};

// material define this part
void getMaterialParameters(in MaterialInputParameters inputParam, out MaterialOutputParameters outputParam)
{

}

void main()
{
    MaterialInputParameters inputParam;
    MaterialOutputParameters outputParam;
    getMaterialParameters(inputParam, outputParam);
}