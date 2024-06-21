#version 430 core
in vec3 osg_Vertex;
in vec3 osg_Normal;
in vec4 osg_Color;
in vec2 osg_MultiTexCoord0;
out V2F
{
    vec3 position;
    vec3 normal;
    vec4 color;
    vec2 texcoord0;
}v2f;

uniform mat4 osg_ModelViewMatrix;
uniform mat4 osg_ProjectionMatrix;
uniform mat3 osg_NormalMatrix;

void main()
{
    vec4 viewSpace = osg_ModelViewMatrix * vec4(osg_Vertex, 1.0);
    gl_Position = osg_ProjectionMatrix * viewSpace;
    v2f.position = viewSpace.xyz;
    v2f.normal = osg_NormalMatrix * osg_Normal;
    v2f.color = osg_Color;
    v2f.texcoord0 = osg_MultiTexCoord0;
}