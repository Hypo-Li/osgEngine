#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "Noise.glsl"
layout(local_size_x = 4, local_size_y = 4, local_size_z = 32) in;
layout(rgba8, binding = 0) uniform image3D uDetailNoiseImage;

#if 0
void main()
{
    vec3 uvw = (gl_GlobalInvocationID.xyz + 0.5) / vec3(32);
    const float cellCount = 2;
    float worley0 = 1.0 - worleyNoise2(uvw, cellCount * 1);
    float worley1 = 1.0 - worleyNoise2(uvw, cellCount * 2);
    float worley2 = 1.0 - worleyNoise2(uvw, cellCount * 4);
    float worley3 = 1.0 - worleyNoise2(uvw, cellCount * 8);
    float wfbm0 = worley0 * 0.625 + worley1 * 0.25 + worley2 * 0.125;
    float wfbm1 = worley1 * 0.625 + worley2 * 0.25 + worley3 * 0.125;
    float wfbm2 = worley2 * 0.75 + worley3 * 0.25;
     imageStore(uDetailNoiseImage, ivec3(gl_GlobalInvocationID.xyz), vec4(wfbm0, wfbm1, wfbm2, 1.0));
}
#else
void main()
{
    const float freq = 8.0;
    vec3 uvw = (gl_GlobalInvocationID.xyz + 0.5) / vec3(32);
    vec3 wfbm = vec3(
        worleyFbm(uvw, freq),
        worleyFbm(uvw, freq * 2.0),
        worleyFbm(uvw, freq * 4.0)
    );
    imageStore(uDetailNoiseImage, ivec3(gl_GlobalInvocationID.xyz), vec4(wfbm, 1.0));
}
#endif