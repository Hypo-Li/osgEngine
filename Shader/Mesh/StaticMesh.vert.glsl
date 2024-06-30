#version 460 core
layout(location = 0) in vec3 iPosition;
layout(location = 2) in vec3 iNormal;
layout(location = 3) in vec4 iColor;
layout(location = 6) in vec4 iTangent;
layout(location = 8) in vec2 iTexcoord0;
layout(location = 9) in vec2 iTexcoord1;

out V2F
{
    vec3 fragPosVS;
    vec3 normalWS;
    vec4 tangentWS;
    vec4 color;
    vec2 texcoord0;
    vec2 texcoord1;
} v2f;

uniform mat4 osg_ModelViewMatrix;
uniform mat4 osg_ProjectionMatrix;
uniform mat3 osg_NormalMatrix;
uniform mat4 osg_ViewMatrixInverse;

void main()
{
    vec4 fragPosVS = osg_ModelViewMatrix * vec4(iPosition, 1.0);
    gl_Position = osg_ProjectionMatrix * fragPosVS;
    v2f.fragPosVS = fragPosVS.xyz;
    vec3 normalVS = osg_NormalMatrix * iNormal;
    vec3 tangentVS = osg_NormalMatrix * iTangent.xyz;
    v2f.normalWS = mat3(osg_ViewMatrixInverse) * normalVS;
    v2f.tangentWS = vec4(mat3(osg_ViewMatrixInverse) * tangentVS, iTangent.w);
    v2f.color = iColor;
    v2f.texcoord0 = iTexcoord0;
    v2f.texcoord1 = iTexcoord1;
}
