#version 460 core
layout(local_size_x = 32, local_size_y = 32) in;
layout(binding = 0, rgba16f) uniform imageCube uEnvCubemapImage;
uniform sampler2D uEnvMapTexture;
const float PI = 3.14159265358979323846;
const float TAU = 2.0 * PI;
#define MAX_LUM 500.0

vec3 getSamplingVector()
{
    vec2 st = vec2(gl_GlobalInvocationID.xy + 0.5) / vec2(imageSize(uEnvCubemapImage));
    vec2 uv = 2.0 * vec2(st.x, 1.0 - st.y) - 1.0;
    vec3 dir[6] = {
        vec3(1.0, uv.y, -uv.x),
        vec3(-1.0, uv.y, uv.x),
        vec3(uv.x, 1.0, -uv.y),
        vec3(uv.x, -1.0, uv.y),
        vec3(uv.x, uv.y, 1.0),
        vec3(-uv.x, uv.y, -1.0)
    };
    return normalize(dir[gl_GlobalInvocationID.z]);
}

void main()
{
    vec3 v = getSamplingVector();
    vec2 uv = vec2(
        atan(v.z, v.x) / TAU + 0.5,
        asin(v.y) / PI + 0.5
    );
    vec3 color = min(texture(uEnvMapTexture, uv).rgb, vec3(MAX_LUM));
    imageStore(uEnvCubemapImage, ivec3(gl_GlobalInvocationID), vec4(color, 1.0));
}