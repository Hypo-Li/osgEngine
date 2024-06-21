#version 430 core
in vec3 osg_Vertex;
in vec3 osg_Normal;
uniform mat4 osg_ModelViewProjectionMatrix;
uniform uint uCascadeIndex;

const vec3 lightDir = vec3(0.7071, 0.0, 0.7071);

const uint MAX_SHADOW_CASCADE_COUNT = 4;
layout(std140, binding = 1) uniform ShadowParameters
{
    mat4 uViewSpaceToLightSpaceMatrices[MAX_SHADOW_CASCADE_COUNT];
    // x: depth bias, y: slope bias, z: soft transition scale, w: cascade far
    vec4 uCascadeParameters[MAX_SHADOW_CASCADE_COUNT];
    float uReceiverBias;
};

void main()
{
    vec4 clipSpace = osg_ModelViewProjectionMatrix * vec4(osg_Vertex, 1.0);
    float ndl = dot(osg_Normal, lightDir);
    float slope = clamp(abs(ndl) > 0 ? sqrt(clamp(1 - ndl * ndl, 0.0, 1.0)) / ndl : 1.0, 0.0, 1.0);
    float depthBias = uCascadeParameters[uCascadeIndex].x + uCascadeParameters[uCascadeIndex].y * slope;
    clipSpace.z += depthBias;
    gl_Position = clipSpace;
}
