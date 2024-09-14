#version 430 core
in vec2 uv;
out vec4 fragData;
uniform sampler2D uAtmosphereColorTexture;
uniform sampler2D uCloudColorTexture;

// Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
vec3 aces(vec3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
    vec3 atmoColor = texelFetch(uAtmosphereColorTexture, ivec2(gl_FragCoord.xy), 0).rgb;
    vec4 cloudColor = texelFetch(uCloudColorTexture, ivec2(gl_FragCoord.xy), 0);
    vec3 color = atmoColor * cloudColor.a + cloudColor.rgb;
    fragData = vec4(pow(aces(color), vec3(0.454545)), 1.0);
}