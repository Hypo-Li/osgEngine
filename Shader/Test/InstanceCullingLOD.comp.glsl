#version 430 core
layout(local_size_x = 1024) in;
layout(rgba32f, binding = 0) uniform imageBuffer uInstancedDataBuffer;
layout(r32ui, binding = 1) uniform uimageBuffer uInstancedRemapBuffer;
layout(std140, binding = 2) buffer RemapCountSSBO
{
    uint totalRemapCount[4];
};

uniform mat4 osg_ModelViewMatrix;
uniform mat4 osg_ProjectionMatrix;
uniform float uBoundingSphereRadius;
uniform uint uInstanceCount;

uniform vec2 uLOD0Range;
uniform vec2 uLOD1Range;
uniform vec2 uLOD2Range;
uniform vec2 uLOD3Range;

struct Frustum
{
    vec4 planes[6];
};

Frustum getFrustum(in mat4 projMat)
{
    Frustum frustum;
    // left
    frustum.planes[0] = projMat[3] + projMat[0];
    frustum.planes[0] *= 1.0 / length(frustum.planes[0].xyz);
    // right
    frustum.planes[1] = projMat[3] - projMat[0];
    frustum.planes[1] *= 1.0 / length(frustum.planes[1].xyz);
    // bottom
    frustum.planes[2] = projMat[3] + projMat[1];
    frustum.planes[2] *= 1.0 / length(frustum.planes[2].xyz);
    // top
    frustum.planes[3] = projMat[3] - projMat[1];
    frustum.planes[3] *= 1.0 / length(frustum.planes[3].xyz);
    // near
    frustum.planes[4] = projMat[3] + projMat[2];
    frustum.planes[4] *= 1.0 / length(frustum.planes[4].xyz);
    // far
    frustum.planes[5] = projMat[3] - projMat[2];
    frustum.planes[5] *= 1.0 / length(frustum.planes[5].xyz);

    return frustum;
}

// TODO: apply scale to radius
// TODO: support LOD

shared uint sharedRemapCount[1024];
void main()
{
    uint instanceCountPerThread = uint(ceil(uInstanceCount / 1024.0f));
    uint runThreadCount = uint(ceil(uInstanceCount / float(instanceCountPerThread)));
    if (gl_LocalInvocationID.x >= runThreadCount)
        return;

    uint remapTemp[1024];
    uint lodTemp[1024];
    Frustum frustum = getFrustum(transpose(osg_ProjectionMatrix));
    uint remapCount = 0;
    uint lodRemapCount[4] = {0, 0, 0, 0};
    for (uint i = 0; i < instanceCountPerThread; ++i)
    {
        uint instanceId = gl_LocalInvocationID.x * instanceCountPerThread + i;
        if (instanceId >= uInstanceCount)
            break;
        vec4 m0 = imageLoad(uInstancedDataBuffer, int(instanceId));
        vec4 m1 = imageLoad(uInstancedDataBuffer, int(instanceId * 4 + 1));
        vec4 m2 = imageLoad(uInstancedDataBuffer, int(instanceId * 4 + 2));
        vec4 m3 = imageLoad(uInstancedDataBuffer, int(instanceId * 4 + 3));
        mat4 modelView = osg_ModelViewMatrix * mat4(m0, m1, m2, m3);
        vec4 center = modelView * vec4(0, 0, 0, 1);
        
        bool culled = false;
        for (uint p = 0; p < 6; ++p)
        {
            vec4 plane = frustum.planes[p];
            float dist = dot(plane.xyz, center.xyz) + plane.w;
            culled = culled || bool(dist < -uBoundingSphereRadius);
        }

        float areaRatio = 1.0;
        if (osg_ProjectionMatrix[3][3] == 1.0)
        {
            float xRadius = uBoundingSphereRadius * osg_ProjectionMatrix[0][0] * 0.5;
            float yRadius = uBoundingSphereRadius * osg_ProjectionMatrix[1][1] * 0.5;
            float area = 3.1415926 * xRadius * yRadius;
            areaRatio = area * 0.25;
        }
        else
        {
            vec4 clipSpace = osg_ProjectionMatrix * modelView * vec4(0, 0, 0, 1);
            float xRadius = uBoundingSphereRadius * osg_ProjectionMatrix[0][0] / clipSpace.w;
            float yRadius = uBoundingSphereRadius * osg_ProjectionMatrix[1][1] / clipSpace.w;
            float area = 3.1415926 * xRadius * yRadius;
            areaRatio = area * 0.25;
        }

        uint lod = 0;
        if (areaRatio < uLOD3Range.x)
            culled = true;
        else if (areaRatio < uLOD3Range.y)
            lod = 3;
        else if (areaRatio < uLOD2Range.y)
            lod = 2;
        else if (areaRatio < uLOD1Range.y)
            lod = 1;

        //culled = culled || (areaRatio < 0.00005);

        if (!culled)
        {
            remapTemp[remapCount] = instanceId;
            lodTemp[remapCount] = lod;
            remapCount++;
            lodRemapCount[lod]++;
        }
    }
    sharedRemapCount[gl_LocalInvocationID.x] = remapCount;

    barrier();

    uint remapOffset = 0;
    for (uint i = 0; i < gl_LocalInvocationID.x; ++i)
    {
        remapOffset += sharedRemapCount[i];
    }

    if (gl_LocalInvocationID.x == runThreadCount - 1)
    {
        totalRemapCount[0] = remapOffset + lodRemapCount[0];
        totalRemapCount[1] = remapOffset + lodRemapCount[1];
        totalRemapCount[2] = remapOffset + lodRemapCount[2];
        totalRemapCount[3] = remapOffset + lodRemapCount[3];
    }

    for (uint i = 0; i < remapCount; ++i)
    {
        imageStore(uInstancedRemapBuffer, int(remapOffset + i), uvec4(remapTemp[i]));
    }
}