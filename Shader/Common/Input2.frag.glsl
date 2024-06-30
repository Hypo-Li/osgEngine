#version 430 core
in V2F
{
    vec3 position;
    vec3 normal;
    vec4 color;
    vec2 texcoord0;
}v2f;
out vec4 fragData;

void main()
{
    fragData = vec4(0.0, 1.0, 0.0, 1.0);
}