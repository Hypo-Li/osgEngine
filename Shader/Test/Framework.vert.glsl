#version 430 core
#pragma import_defines(INSTANCED, SHADOW_CAST, ALPHA_MODE)
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

#define ALPHA_MODE_OPAQUE 0
#define ALPHA_MODE_MASK 1
#define ALPHA_MODE_BLEND 2

#ifndef ALPHA_MODE
    #define ALPHA_MODE ALPHA_MODE_OPAQUE
#endif

#if INSTANCED
uniform samplerBuffer uInstancedDataBuffer;
uniform usamplerBuffer uInstancedRemapBuffer;
#endif

out V2F
{
    vec3 fragPosVS;
    vec3 normalVS;
    vec4 tangentVS;
    vec4 color;
    vec4 texcoord0;
    vec4 texcoord1;
    vec4 fragPosCS;
    vec4 prevFragPosCS;
} v2f;

uniform mat4 osg_ModelViewMatrix;
uniform mat4 osg_ProjectionMatrix;
uniform mat4 osg_ViewMatrixInverse;
uniform mat3 osg_NormalMatrix;
uniform mat4 uPrevFrameMVPMatrix;

layout(std140, binding = 0) uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uInverseViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uInverseProjectionMatrix;
    mat4 uReprojectionMatrix;
    vec2 uJitterPixels;
    vec2 uPrevJitterPixels;
};

// TODO: fix instanced normal
void main()
{
#if INSTANCED
    uint instanceId = texelFetch(uInstancedRemapBuffer, gl_InstanceID).x;
    vec4 m0 = texelFetch(uInstancedDataBuffer, int(instanceId) * 4);
    vec4 m1 = texelFetch(uInstancedDataBuffer, int(instanceId) * 4 + 1);
    vec4 m2 = texelFetch(uInstancedDataBuffer, int(instanceId) * 4 + 2);
    vec4 m3 = texelFetch(uInstancedDataBuffer, int(instanceId) * 4 + 3);
    vec4 viewSpace = osg_ModelViewMatrix * mat4(m0, m1, m2, m3) * iPosition;
#else
    vec4 viewSpace = osg_ModelViewMatrix * iPosition;
#endif
    
    gl_Position = uProjectionMatrix * viewSpace;
    v2f.fragPosVS = viewSpace.xyz;
    v2f.normalVS = osg_NormalMatrix * iNormal;
    v2f.tangentVS = vec4(osg_NormalMatrix * iTangent.xyz, iTangent.w);
    v2f.color = iColor;
    v2f.texcoord0 = iTexcoord0;
    v2f.texcoord1 = iTexcoord1;

    v2f.fragPosCS = uProjectionMatrix * viewSpace;
    v2f.prevFragPosCS = uPrevFrameMVPMatrix * iPosition;
}
