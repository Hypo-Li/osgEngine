#version 430 core
in vec4 osg_Vertex;
in vec3 osg_Normal;
in vec4 osg_Color;
in vec4 osg_MultiTexCoord0;
in vec4 osg_MultiTexCoord1;
layout(location = 11) in vec4 osg_Tangent;

out V2F
{
    vec3 fragPosVS;
    vec3 normalWS;
    vec4 tangentWS;
    vec4 color;
    vec4 texcoord0;
    vec4 texcoord1;
} v2f;

uniform mat4 osg_ModelViewMatrix;
uniform mat4 osg_ProjectionMatrix;
uniform mat4 osg_ViewMatrixInverse;
uniform mat3 osg_NormalMatrix;

void main()
{
    vec4 viewSpace = osg_ModelViewMatrix * osg_Vertex;
    gl_Position = osg_ProjectionMatrix * viewSpace;
    v2f.fragPosVS = viewSpace.xyz;
    v2f.normalWS = mat3(osg_ViewMatrixInverse) * osg_NormalMatrix * osg_Normal;
    v2f.tangentWS = vec4(mat3(osg_ViewMatrixInverse) * osg_NormalMatrix * osg_Tangent.xyz, osg_Tangent.w);
    v2f.color = osg_Color;
    v2f.texcoord0 = osg_MultiTexCoord0;
    v2f.texcoord0 = osg_MultiTexCoord1;
}
