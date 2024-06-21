#version 430 core
in V2F
{
    vec3 position;
    vec3 normal;
    vec4 color;
    vec2 texcoord0;
}v2f;
out vec4 fragData;
uniform mat4 osg_ViewMatrix;
const vec3 sunDir = normalize(vec3(1.0, -1.0, 1.0));
void main()
{
    //vec3 normal = normalize(v2f.normal);
    //vec3 viewDir = normalize(-v2f.position);
    //vec3 lightDir = mat3(osg_ViewMatrix) * sunDir;
    //float ndl = max(dot(normal, lightDir), 0.0);
    //fragData = vec4((ndl + 0.01) * v2f.color);
}