//#version 430 core
in V2F
{
    vec3 position;
    vec3 normal;
    vec4 tangent;
    vec2 texcoord0;
    vec4 color;
}v2f;
out vec4 fragData[5];
uniform sampler2D DiffuseMap;
uniform sampler2D NormalMap;
uniform sampler2D ShininessMap;
uniform sampler2D EmissiveMap;
// for car model
uniform bool uEnableCustomColor;
uniform vec4 gradeColor;


uniform vec3 uEmissiveFactor;

vec3 srgb_to_linear(vec3 srgb) { return pow(srgb, vec3(2.2)); }

void main()
{
    vec4 baseColor = texture(DiffuseMap, v2f.texcoord0);
    if (baseColor.a < 0.5)
        discard;
    if (uEnableCustomColor)
        baseColor.rgb *= gradeColor.rgb;
    vec2 metallicRoughness = texture(ShininessMap, v2f.texcoord0).bg;
    vec4 normalValue = texture(NormalMap, v2f.texcoord0);
    vec3 normal = normalize(v2f.normal);
    if (length(v2f.tangent.xyz) > 0.0 && normalValue.a == 1.0)
    {
        vec3 tangent = normalize(v2f.tangent.xyz);
        vec3 bitangent = cross(tangent, normal) * v2f.tangent.w;
        mat3 tbn = mat3(tangent, bitangent, normal);
        normal = tbn * normalize(normalValue.xyz * 2.0 - 1.0);
    }
    vec3 emissive = srgb_to_linear(texture(EmissiveMap, v2f.texcoord0).rgb) * uEmissiveFactor;
    
    fragData[0] = vec4(emissive, 1.0);
    fragData[1] = vec4(normal, 1.0);
    fragData[2] = vec4(baseColor.rgb * v2f.color.rgb, 1.0);
    fragData[3] = vec4(metallicRoughness, 1.0, 1.0);
    fragData[4] = vec4(v2f.position, 1.0);
}