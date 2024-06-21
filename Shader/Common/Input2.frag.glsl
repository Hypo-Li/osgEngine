#version 430 core
in vec2 uv;
out vec4 fragData[2];
uniform vec4 uResolution;
in V2F
{
    vec3 position;
    vec3 normal;
    vec4 color;
    vec2 texcoord0;
}v2f;

const vec3 lightDir = vec3(0.7071, 0.0, 0.7071);

void main()
{
    vec3 normal = normalize(v2f.normal);
    float ndl = max(dot(normal, lightDir), 0.0);
    fragData[0] = vec4(vec3(pow(ndl, 0.454545)), 1.0);
    fragData[1] = vec4(normal, 0.0);
}