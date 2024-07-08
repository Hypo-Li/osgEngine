#version 430 core
#extension GL_GOOGLE_include_directive : enable
#include "Noise.glsl"
layout(local_size_x = 4, local_size_y = 4, local_size_z = 32) in;
layout(rgba8, binding = 0) uniform image3D uDetailNoiseImage;

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