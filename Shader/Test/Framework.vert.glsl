#version 430 core
#pragma import_defines(INSTANCED, SHADOW_CAST)
// in vec4 osg_Vertex;
// in vec3 osg_Normal;
// in vec4 osg_Color;
// in vec4 osg_MultiTexCoord0;
// in vec4 osg_MultiTexCoord1;
// layout(location = 6) in vec4 osg_Tangent;
layout(location = 0) in vec4 iPosition;
layout(location = 1) in vec4 iColor;
layout(location = 2) in vec3 iNormal;
layout(location = 3) in vec4 iTangent;
layout(location = 4) in vec4 iTexcoord0;
layout(location = 5) in vec4 iTexcoord1;

#ifndef INSTANCED
#define INSTANCED 0
#endif

#ifndef SHADOW_CAST
#define SHADOW_CAST 0
#endif

#if INSTANCED
uniform samplerBuffer uInstancedData;
#endif

out V2F
{
    vec3 fragPosVS;
    vec3 normalVS;
    vec4 tangentVS;
    vec4 color;
    vec4 texcoord0;
    vec4 texcoord1;
} v2f;

uniform mat4 osg_ModelViewMatrix;
uniform mat4 osg_ProjectionMatrix;
uniform mat4 osg_ViewMatrixInverse;
uniform mat3 osg_NormalMatrix;

// TODO: fix instanced normal
void main()
{
#if INSTANCED
    vec4 m0 = texelFetch(uInstancedData, gl_InstanceID * 4);
    vec4 m1 = texelFetch(uInstancedData, gl_InstanceID * 4 + 1);
    vec4 m2 = texelFetch(uInstancedData, gl_InstanceID * 4 + 2);
    vec4 m3 = texelFetch(uInstancedData, gl_InstanceID * 4 + 3);
    vec4 viewSpace = osg_ModelViewMatrix * mat4(m0, m1, m2, m3) * iPosition;
#else
    vec4 viewSpace = osg_ModelViewMatrix * iPosition;
#endif
    
    gl_Position = osg_ProjectionMatrix * viewSpace;
    v2f.fragPosVS = viewSpace.xyz;
    v2f.normalVS = osg_NormalMatrix * iNormal;
    v2f.tangentVS = vec4(osg_NormalMatrix * iTangent.xyz, iTangent.w);
    v2f.color = iColor;
    v2f.texcoord0 = iTexcoord0;
    v2f.texcoord1 = iTexcoord1;
}
