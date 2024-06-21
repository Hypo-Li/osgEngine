#version 430 core
in vec2 uv;
out vec4 fragData;
uniform sampler3D uNoise1Texture;
uniform sampler3D uNoise2Texture;
uniform float osg_FrameTime;

layout(std140, binding = 0) uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uInverseViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uInverseProjectionMatrix;
};

float remap(float x, float a, float b, float c, float d)
{
    return (((x - a) / (b - a)) * (d - c)) + c;
}

void main()
{
    vec4 viewSpace = uInverseProjectionMatrix * vec4(uv * 2.0 - 1.0, 1.0, 1.0);
    viewSpace *= 1.0 / viewSpace.w;
    vec4 worldSpace = uInverseViewMatrix * viewSpace;
    vec3 cameraPosition = vec3(uInverseViewMatrix[3]);
    vec3 viewDirection = normalize(worldSpace.xyz - cameraPosition);
    
    float z = mod(osg_FrameTime * 0.1, 1.0);
    vec3 uvw = vec3(uv + z, z);
    vec4 basicNoise = textureLod(uNoise1Texture, uvw, 0.0);
    float basicFBM = basicNoise.g * 0.625 + basicNoise.b * 0.250 + basicNoise.a * 0.125;
    float cloud = remap(basicNoise.r, basicFBM - 1.0, 1.0, 0.0, 1.0);
    cloud = remap(cloud, 0.8, 1.0, 0.0, 1.0);

    vec3 detailNoise = textureLod(uNoise2Texture, uvw, 0.0).rgb;
    float detailFBM = detailNoise.r * 0.625 + detailNoise.g * 0.250 + detailNoise.b * 0.125;
    cloud = remap(cloud, detailFBM * 0.2, 1.0, 0.0, 1.0);

    fragData = vec4(viewDirection * 0.5 + 0.5, 1.0);
}