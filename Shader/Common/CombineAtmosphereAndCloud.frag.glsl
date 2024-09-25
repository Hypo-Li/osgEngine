#version 430 core
in vec2 uv;
out vec4 fragData;
uniform sampler2D uAtmosphereColorTexture;
uniform sampler2D uCloudColorTexture;

layout(std140, binding = 0) uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uInverseViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uInverseProjectionMatrix;
};

layout (std140, binding = 1) uniform AtmosphereParameters
{
    vec3 uRayleighScatteringBase;
    float uMieScatteringBase;
    vec3 uOzoneAbsorptionBase;
    float uMieAbsorptionBase;
    float uRayleighDensityH;
    float uMieDensityH;
    float uOzoneCenterHeight;
    float uOzoneThickness;
    vec3 uGroundAlbedo;
    float uGroundRadius;
    vec3 uSunDirection;
    float uAtmosphereRadius;
    vec3 uSunIntensity;
};

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

vec3 getWorldPos(vec3 pos)
{
#ifdef PLANET_TOP_AT_ABSOLUTE_WORLD_ORIGIN
    vec3 worldPos = pos / 1000.0 + vec3(0.0, 0.0, uGroundRadius);
#else
    vec3 worldPos = pos / 1000.0;
#endif
    float viewHeight = length(worldPos);
    vec3 upVector = worldPos / viewHeight;
    return upVector * max(viewHeight, uGroundRadius + 0.005);
}

float saturate(float x) { return clamp(x, 0.0, 1.0); }

float remap(float value, float oldMin, float oldMax, float newMin, float newMax)
{
    return newMin + (saturate((value - oldMin) / (oldMax - oldMin)) * (newMax - newMin));
}

void main()
{
    vec3 worldPos = getWorldPos(uInverseViewMatrix[3].xyz);
    float viewHeight = length(worldPos);
    float cloudFactor = remap(viewHeight, 6400, 6550, 1.0, 0.0);
    vec3 atmoColor = texelFetch(uAtmosphereColorTexture, ivec2(gl_FragCoord.xy), 0).rgb;
    vec4 cloudColor = texelFetch(uCloudColorTexture, ivec2(gl_FragCoord.xy), 0);
    vec3 color = atmoColor * cloudColor.a + cloudColor.rgb;
    color = mix(atmoColor, color, vec3(cloudFactor));
    fragData = vec4(pow(aces(color), vec3(0.454545)), 1.0);
}