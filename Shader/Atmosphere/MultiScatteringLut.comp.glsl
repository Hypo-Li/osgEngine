#version 460 core
#extension GL_GOOGLE_include_directive : enable
layout (local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
layout (r11f_g11f_b10f, binding = 0) uniform image2D uMultiScatteringLutImage;
uniform sampler2D uTransmittanceLutTexture;
#define MULTISCATTERING_LUT
#define BINDING_INDEX 1
#include "AtmosphereCommon.glsl"
#define LUT_WIDTH 32
#define LUT_HEIGHT 32

void main()
{
    vec2 uv = (gl_GlobalInvocationID.xy + 0.5) / vec2(LUT_WIDTH, LUT_HEIGHT);
    float sunZenithCos = uv.x * 2.0 - 1.0;
    vec3 sunDir = vec3(0.0, sqrt(1.0 - sunZenithCos * sunZenithCos), sunZenithCos);
    float viewHeight = uGroundRadius + uv.y * (uAtmosphereRadius - uGroundRadius);
    vec3 worldPos = vec3(0.0, 0.0, viewHeight);
    vec3 worldDir = vec3(0.0, 0.0, 1.0);

    const float sphereSolidAngle = 4.0 * PI;
    const float isotropicPhase = 1.0 / sphereSolidAngle;
    vec3 lum0, lum1, fms0, fms1, trans0, trans1;
    rayMarchAtmosphere(0.3, worldPos, worldDir, sunDir, 15.0, 9000000.0, lum0, fms0, trans0);
    rayMarchAtmosphere(0.3, worldPos, -worldDir, sunDir, 15.0, 9000000.0, lum1, fms1, trans1);
    vec3 integratedIlluminance = (sphereSolidAngle * 0.5) * (lum0 + lum1);
    vec3 multiScatAs1 = 0.5 * (fms0 + fms1);
    vec3 inScatteredLuminance = integratedIlluminance * isotropicPhase;
    vec3 multiScatAs1SQR = multiScatAs1 * multiScatAs1;
    vec3 L = inScatteredLuminance * (1.0 + multiScatAs1 + multiScatAs1SQR + multiScatAs1 * multiScatAs1SQR + multiScatAs1SQR * multiScatAs1SQR);
    imageStore(uMultiScatteringLutImage, ivec2(gl_GlobalInvocationID.xy), vec4(L, 1.0));
}