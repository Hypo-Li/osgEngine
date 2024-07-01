#version 460 core
layout(local_size_x = 32, local_size_y = 32) in;
layout(rgba16f, binding = 0) uniform image2D uImage;

void main()
{
    imageStore(uImage, ivec2(gl_GlobalInvocationID.xy), vec4(gl_GlobalInvocationID.xy / vec2(32.0), 0.0, 1.0));
}