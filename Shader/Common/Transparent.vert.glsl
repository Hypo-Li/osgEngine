#version 430 core
in vec4 osg_Vertex;
in vec3 osg_Normal;
in vec4 osg_Color;
out vec3 v2f_fragPos;
out vec3 v2f_normal;
out vec4 v2f_color;
out float v2f_earthNdcZ;
out float v2f_totalNdcZ;
uniform mat3 osg_NormalMatrix;
uniform mat4 osg_ModelViewMatrix;
uniform mat4 osg_ProjectionMatrix;

layout(std140, binding = 0) uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uInverseViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uInverseProjectionMatrix;
    vec4 uEarthNearFar;
    vec4 uGBufferNearFar;
    vec4 uForwardOpaqueNearFar;
    vec4 uTotalNearFar;
};

void main()
{
    vec4 viewSpace = osg_ModelViewMatrix * osg_Vertex;
    gl_Position = osg_ProjectionMatrix * viewSpace;
    v2f_fragPos = viewSpace.xyz;
    v2f_normal = osg_NormalMatrix * osg_Normal;
    v2f_color = osg_Color;
    v2f_earthNdcZ = -uEarthNearFar.z - uEarthNearFar.w / viewSpace.z;
    v2f_totalNdcZ = -uTotalNearFar.z - uTotalNearFar.w / viewSpace.z;
}