#version 460 core
#extension GL_GOOGLE_include_directive : enable
#include "Noise.glsl"
layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
layout(rgba8, binding = 0) uniform image2D uCloudMapImage;

void main()
{
    const float freq = 4.0;
    vec3 uvw = (gl_GlobalInvocationID.xyz + 0.5) / vec3(256);
    float pfbm = perlinFbm(uvw, freq, 7);
    vec3 wfbm = vec3(
        worleyFbm(uvw, freq),
        worleyFbm(uvw, freq * 2.0),
        worleyFbm(uvw, freq * 4.0)
    );
    float perlinWorley = remap(pfbm, -1.0, 1.0, wfbm.x, 1.0);
    imageStore(uCloudMapImage, ivec2(gl_GlobalInvocationID.xy), vec4(pfbm, perlinWorley, 0.0, 1.0));
}