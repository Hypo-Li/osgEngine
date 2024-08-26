#version 460 core
#pragma import_defines(COLOR_GRADING_INDEX)
in vec2 uv;
out vec4 fragData[8];
#define LUT_WIDTH 32
#define LUT_HEIGHT 32
#define LUT_DEPTH 32
#define COLOR_GRADING_LAYER 8

void main()
{
    for (uint i = 0; i < COLOR_GRADING_LAYER; ++i)
    {
        #ifdef COLOR_GRADING_INDEX
        vec3 uvw = vec3(uv, (COLOR_GRADING_INDEX * COLOR_GRADING_LAYER + i + 0.5) / LUT_DEPTH);
        #else
        vec3 uvw = vec3(uv, (i + 0.5) / LUT_DEPTH);
        #endif
        fragData[i] = vec4(uvw, 1.0);
    }
}