#version 430 core
in V2F
{
    vec3 position;
    vec3 normal;
    vec4 color;
    vec2 texcoord0;
    vec2 currentFrameNormalizedScreenSpace;
    vec2 preFrameNormalizedScreenSpace;
}v2f;
out vec4 fragData[2];

uniform uint osg_FrameNumber;

void main()
{
    vec3 viewDir = normalize(-v2f.position);
    vec3 normal = normalize(v2f.normal);
    float ndv = max(dot(normal, viewDir), 0.0);
    fragData[0] = v2f.color;
    fragData[1] = vec4(v2f.currentFrameNormalizedScreenSpace - v2f.preFrameNormalizedScreenSpace, 0.0, 0.0);
}