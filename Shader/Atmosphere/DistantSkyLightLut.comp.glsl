#version 460 core
#extension GL_GOOGLE_include_directive : enable
layout (local_size_x = 1, local_size_y = 1, local_size_z = 64) in;
layout (r11f_g11f_b10f, binding = 0) uniform image2D uDistantSkyLightLutImage;
uniform sampler2D uTransmittanceLutTexture;
uniform sampler2D uMultiScatteringLutTexture;

#define DISTANT_SKYLIGHT_LUT
#define BINDING_INDEX 1
#include "AtmosphereCommon.glsl"

shared vec3 groupSkyLuminanceSamples[64];

void main()
{
    uint linearIndex = gl_GlobalInvocationID.z;

    vec3 samplePos = vec3(0, 0, uGroundRadius + 6.0);
    float viewHeight = length(samplePos);

    vec3 sampleDir;
    {
        const float sqrtSample = 8;
        float i = 0.5 + float(linearIndex / 8);
        float j = 0.5 + float(linearIndex - float((linearIndex / 8)*8));
        float randA = i / sqrtSample;;
        float randB = j / sqrtSample;
        float theta = 2.0 * PI * randA;
        float cosTheta = cos(theta);
        float sinTheta = sin(theta);
        float phi = acos(1.0 - 2.0 * randB);
        float cosPhi = cos(phi);
        float sinPhi = sin(phi);
        sampleDir.x = cosTheta * sinPhi;
        sampleDir.y = sinTheta * sinPhi;
        sampleDir.z = cosPhi;
    }

    vec3 lum, fms, trans;
    rayMarchAtmosphere(0.3, samplePos, sampleDir, uSunDirection, 10.0, 9000000, lum, fms, trans);

    groupSkyLuminanceSamples[linearIndex] = lum;
    barrier();

    if (linearIndex < 32)
        groupSkyLuminanceSamples[linearIndex] += groupSkyLuminanceSamples[linearIndex + 32];
    barrier();

    if (linearIndex < 16)
        groupSkyLuminanceSamples[linearIndex] += groupSkyLuminanceSamples[linearIndex + 16];
    barrier();

    if (linearIndex < 8)
        groupSkyLuminanceSamples[linearIndex] += groupSkyLuminanceSamples[linearIndex + 8];
    barrier();

    if (linearIndex < 4)
        groupSkyLuminanceSamples[linearIndex] += groupSkyLuminanceSamples[linearIndex + 4];
    barrier();

    if (linearIndex < 2)
        groupSkyLuminanceSamples[linearIndex] += groupSkyLuminanceSamples[linearIndex + 2];
    if (linearIndex < 1)
    {
        vec3 accumulatedLuminanceSamples = groupSkyLuminanceSamples[0] + groupSkyLuminanceSamples[1];
        const float samplerSolidAngle = 1.0 / 64.0;
		const vec3 illuminance = accumulatedLuminanceSamples * samplerSolidAngle;
		imageStore(uDistantSkyLightLutImage, ivec2(0), vec4(illuminance, 1.0));
    }
}