#version 430 core
layout(location = 0) in vec3 iPosition;
out vec2 uv;

void main()
{
    gl_Position = vec4(iPosition, 1.0);
    uv = iPosition.xy * 0.5 + 0.5;
}