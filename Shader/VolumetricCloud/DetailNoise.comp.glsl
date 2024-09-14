#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "Noise.glsl"
layout(local_size_x = 4, local_size_y = 4, local_size_z = 32) in;
layout(rgba8, binding = 0) uniform image3D uDetailNoiseImage;

void main()
{
    vec3 uvw = (gl_GlobalInvocationID.xyz + 0.5) / vec3(32);
    // float worley0 = worleyNoise(uvw * 2.0, 2.0);
    // float worley1 = worleyNoise(uvw * 4.0, 4.0);
    // float worley2 = worleyNoise(uvw * 8.0, 8.0);
    // float worley3 = worleyNoise(uvw * 16.0, 16.0);

    float worley0 = worleyNoise(uvw * 4.0, 4.0);
    float worley1 = worleyNoise(uvw * 8.0, 8.0);
    float worley2 = worleyNoise(uvw * 16.0, 16.0);
    float worley3 = worleyNoise(uvw * 32.0, 32.0);

    float wfbm0 = worley0 * 0.625 + worley1 * 0.25 + worley2 * 0.125;
    float wfbm1 = worley1 * 0.625 + worley2 * 0.25 + worley3 * 0.125;
    float wfbm2 = worley2 * 0.75 + worley3 * 0.25;
     imageStore(uDetailNoiseImage, ivec3(gl_GlobalInvocationID.xyz), vec4(wfbm0, wfbm1, wfbm2, 1.0));
}