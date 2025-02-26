#version 460 core
#extension GL_GOOGLE_include_directive : enable
#include "Noise.glsl"
layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
layout(rgba16f, binding = 0) uniform image2D uCloudMapImage;

void main()
{
    const float freq = 4.0;
    vec3 uvw = vec3((gl_GlobalInvocationID.xy + 0.5) / vec2(512), 0.5);
    float pfbm = mix(1.0, perlinFbm(uvw, 4.0, 7), 0.5);
    pfbm = abs(pfbm * 2.0 - 1.0);
    
    float worley0 = worleyFbm(uvw, freq * 1);
    float worley1 = worleyFbm(uvw, freq * 2);
    float worley2 = worleyFbm(uvw, freq * 4);
    float wfbm = dot(vec3(worley0, worley1, worley2), vec3(0.625, 0.25, 0.125));
    float perlinWorley = remap(pfbm, 0.0, 1.0, worley0, 1.0);

    float coverage = remap(perlinWorley, wfbm - 1.0, 1.0, 0.0, 1.0);
    coverage = clamp(remap(coverage, 0.85, 0.95, 0.0, 1.0), 0.0, 1.0);
    float cloudType = clamp(remap(pfbm, 0.0, 1.0, 0.2, 1.0), 0.0, 1.0);

    imageStore(uCloudMapImage, ivec2(gl_GlobalInvocationID.xy), vec4(coverage));
}