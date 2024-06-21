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
    vec2 currentFrameNormalizedScreenSpace;
    vec2 preFrameNormalizedScreenSpace;
}v2f;

layout(std140, binding = 0) uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uProjectionMatrixJittered;
    mat4 uInverseViewMatrix;
    mat4 uInverseProjectionMatrix;
    mat4 uInverseProjectionMatrixJittered;
    mat4 uReprojectionMatrix;
};
layout(location = 0) uniform mat4 uPreModelCurrentViewMatrix;
uniform mat4 osg_ModelViewMatrix;
uniform mat4 osg_ProjectionMatrix;
uniform mat3 osg_NormalMatrix;

void main()
{
    vec4 viewSpace = osg_ModelViewMatrix * vec4(osg_Vertex, 1.0);
    vec4 currentFrameNdcSpace = uProjectionMatrix * viewSpace;
    currentFrameNdcSpace *= 1.0 / currentFrameNdcSpace.w;
    vec4 preFrameNdcSpace = uProjectionMatrix * uPreModelCurrentViewMatrix * vec4(osg_Vertex, 1.0);
    preFrameNdcSpace *= 1.0 / preFrameNdcSpace.w;

    gl_Position = uProjectionMatrixJittered * viewSpace;
    v2f.position = viewSpace.xyz;
    v2f.normal = osg_NormalMatrix * osg_Normal;
    v2f.color = osg_Color;
    v2f.texcoord0 = osg_MultiTexCoord0;
    v2f.currentFrameNormalizedScreenSpace = currentFrameNdcSpace.xy * 0.5 + 0.5;
    v2f.preFrameNormalizedScreenSpace = preFrameNdcSpace.xy * 0.5 + 0.5;
}