#version 460 core
in vec3 uvw;
out vec4 fragData;
#define LUT_WIDTH 32
#define LUT_HEIGHT 32
#define LUT_DEPTH 32

void main()
{
    fragData = vec4(uvw, 1.0);
}