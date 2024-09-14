#version 460 core
#extension GL_GOOGLE_include_directive : enable
#include "Noise.glsl"
layout(local_size_x = 4, local_size_y = 4, local_size_z = 64) in;
layout(rgba8, binding = 0) uniform image3D uBasicNoiseImage;

const float frequenceMul[6] = { 2.0, 8.0, 14.0, 20.0, 26.0, 32.0 };

void main()
{
    vec3 uvw = (gl_GlobalInvocationID.xyz + 0.5) / vec3(128);
    float pfbm = perlinFbm(uvw, 8.0, 3) * 0.5 + 0.5;
    float perlinWorley = 0.0;
    {
        vec3 worley = vec3(
            worleyNoise(uvw * 8.0, 8.0),
            worleyNoise(uvw * 32.0, 32.0),
            worleyNoise(uvw * 56.0, 56.0)
        );
        float wfbm = dot(worley, vec3(0.625, 0.25, 0.125));
        perlinWorley = remap(pfbm, 0.0, 1.0, wfbm, 1.0);
    }

    float worley0 = worleyNoise(uvw * 4.0, 4.0);
    float worley1 = worleyNoise(uvw * 8.0, 8.0);
    float worley2 = worleyNoise(uvw * 16.0, 16.0);
    float worley3 = worleyNoise(uvw * 32.0, 32.0);
    float worley4 = worleyNoise(uvw * 64.0, 64.0);

    float wfbm0 = worley1 * 0.625 + worley2 * 0.25 + worley3 * 0.125;
    float wfbm1 = worley2 * 0.625 + worley3 * 0.25 + worley4 * 0.125;
    float wfbm2 = worley3 * 0.75 + worley4 * 0.25;

    imageStore(uBasicNoiseImage, ivec3(gl_GlobalInvocationID.xyz), vec4(perlinWorley, wfbm0, wfbm1, wfbm2));
}