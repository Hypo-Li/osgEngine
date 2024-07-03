#version 460 core
#extension GL_GOOGLE_include_directive : enable
layout (local_size_x = 1, local_size_y = 1, local_size_z = 64) in;
layout (rgba16f, binding = 0) uniform image2D uMultiScatteringLutImage;
uniform sampler2D uTransmittanceLutTexture;
#define MULTISCATTERING_LUT
#define BINDING_INDEX 1
#include "AtmosphereCommon.glsl"
#define LUT_WIDTH 32
#define LUT_HEIGHT 32

shared vec3 lumMem[64];
shared vec3 fmsMem[64];

void main()
{
    vec2 uv = (gl_GlobalInvocationID.xy + 0.5) / vec2(LUT_WIDTH, LUT_HEIGHT);
    float sunZenithCos = uv.x * 2.0 - 1.0;
    vec3 sunDir = vec3(0.0, sqrt(1.0 - sunZenithCos * sunZenithCos), sunZenithCos);
    float viewHeight = uGroundRadius + uv.y * (uAtmosphereRadius - uGroundRadius);
    vec3 worldPos = vec3(0.0, 0.0, viewHeight);
    vec3 worldDir = vec3(0.0, 0.0, 1.0);
#define SQRTSAMPLECOUNT 8
    const float sqrtSample = SQRTSAMPLECOUNT;
    const float invSample = 1.0 / (sqrtSample * sqrtSample);
    uint z = gl_GlobalInvocationID.z;
    {
        float i = 0.5 + float(z / SQRTSAMPLECOUNT);
        float j = 0.5 + float(z - float((z / SQRTSAMPLECOUNT)*SQRTSAMPLECOUNT));
        float randA = i / sqrtSample;;
        float randB = j / sqrtSample;
        float theta = 2.0 * PI * randA;
        float cosTheta = cos(theta);
        float sinTheta = sin(theta);
        float phi = acos(1.0 - 2.0 * randB);
        float cosPhi = cos(phi);
        float sinPhi = sin(phi);
        worldDir.x = cosTheta * sinPhi;
        worldDir.y = sinTheta * sinPhi;
        worldDir.z = cosPhi;
        vec3 lum, fms, trans;
        rayMarchAtmosphere(0.3, worldPos, worldDir, sunDir, 20.0, 9000000.0, lum, fms, trans);
        lumMem[z] = lum * invSample;
        fmsMem[z] = fms * invSample;
    }
    barrier();
    if (z < 32)
    {
        lumMem[z] += lumMem[z + 32];
        fmsMem[z] += fmsMem[z + 32];
    }
    barrier();
    if (z < 16)
    {
        lumMem[z] += lumMem[z + 16];
        fmsMem[z] += fmsMem[z + 16];
    }
    barrier();
    if (z < 8)
    {
        lumMem[z] += lumMem[z + 8];
        fmsMem[z] += fmsMem[z + 8];
    }
    barrier();
    if (z < 4)
    {
        lumMem[z] += lumMem[z + 4];
        fmsMem[z] += fmsMem[z + 4];
    }
    barrier();
    if (z < 2)
    {
        lumMem[z] += lumMem[z + 2];
        fmsMem[z] += fmsMem[z + 2];
    }
    barrier();
    if (z < 1)
    {
        lumMem[z] += lumMem[z + 1];
        fmsMem[z] += fmsMem[z + 1];
    }
    barrier();
    if (z > 0)
        return;
    imageStore(uMultiScatteringLutImage, ivec2(gl_GlobalInvocationID.xy), vec4(lumMem[0] / (1.0 - fmsMem[0]), 1.0));
}

// #version 460 core
// #extension GL_GOOGLE_include_directive : enable
// layout (local_size_x = 1, local_size_y = 1, local_size_z = 64) in;
// layout (rgba16f, binding = 0) uniform image2D uMultiScatteringLutImage;
// uniform sampler2D uTransmittanceLutTexture;
// #define MULTISCATTERING_LUT
// #define BINDING_INDEX 1
// #include "AtmosphereCommon.glsl"
// #define LUT_WIDTH 32
// #define LUT_HEIGHT 32

// void main()
// {
//     vec2 uv = (gl_GlobalInvocationID.xy + 0.5) / vec2(LUT_WIDTH, LUT_HEIGHT);
//     float sunZenithCos = uv.x * 2.0 - 1.0;
//     vec3 sunDir = vec3(0.0, sqrt(1.0 - sunZenithCos * sunZenithCos), sunZenithCos);
//     float viewHeight = uGroundRadius + uv.y * (uAtmosphereRadius - uGroundRadius);
//     vec3 worldPos = vec3(0.0, 0.0, viewHeight);
//     vec3 worldDir = vec3(0.0, 0.0, 1.0);

//     float sphereSolidAngle = 4.0 * PI;
//     float isotropicPhase = 1.0 / sphereSolidAngle;
//     vec3 multiScatAs10, multiScatAs11, lum0, lum1, fms0, fms1, trans0, trans1;
//     rayMarchAtmosphere(0.3, worldPos, worldDir, sunDir, 15.0, 9000000.0, multiScatAs10, lum0, fms0, trans0);
//     rayMarchAtmosphere(0.3, worldPos, -worldDir, sunDir, 15.0, 9000000.0, multiScatAs11, lum1, fms1, trans1);
//     vec3 integratedIlluminance = (sphereSolidAngle * 0.5) * (lum0 + lum1);
//     vec3 multiScatAs1 = 0.5 * (multiScatAs10 + multiScatAs11);
//     vec3 inScatteredLuminance = integratedIlluminance * isotropicPhase;
//     vec3 multiScatAs1SQR = multiScatAs1 * multiScatAs1;
//     vec3 L = inScatteredLuminance * (1.0 + multiScatAs1 + multiScatAs1SQR + multiScatAs1 + multiScatAs1SQR + multiScatAs1SQR * multiScatAs1SQR);
//     imageStore(uMultiScatteringLutImage, ivec2(gl_GlobalInvocationID.xy), vec4(L, 1.0));
// }