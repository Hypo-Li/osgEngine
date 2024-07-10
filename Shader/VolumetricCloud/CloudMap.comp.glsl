#version 460 core
#extension GL_GOOGLE_include_directive : enable
#include "Noise.glsl"
layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
layout(rgba8, binding = 0) uniform image2D uCloudMapImage;

void main()
{
    const float freq = 4.0;
    vec3 uvw = (gl_GlobalInvocationID.xyz + 0.5) / vec3(256);
    float pfbm = perlinFbm(uvw, 8.0, 3) * 0.5 + 0.5;
    float perlinWorley = 0.0;
    {
        float worley0 = worleyNoise(uvw * 8.0, 8.0);
        float worley1 = worleyNoise(uvw * 32.0, 32.0);
        float worley2 = worleyNoise(uvw * 56.0, 56.0);
        perlinWorley = remap(pfbm, 0.0, 1.0, dot(vec3(worley0, worley1, worley2), vec3(0.625, 0.25, 0.125)), 1.0);
    }

    float coverage = remap(perlinWorley, 0.0, 1.0, 0.2, 1.0);
    float cloudType = clamp(remap(pfbm, 0.0, 1.0, 0.2, 1.0), 0.0, 1.0);

    imageStore(uCloudMapImage, ivec2(gl_GlobalInvocationID.xy), vec4(coverage, cloudType, 0.0, 0.0));
}