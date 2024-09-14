#version 460 core
in vec2 uv;
out vec4 fragData[2];
uniform sampler2D uCurrentCloudColorTexture;
uniform sampler2D uCurrentCloudDistanceTexture;
uniform sampler2D uHistoryCloudColorTexture;
uniform sampler2D uHistoryCloudDistanceTexture;
uniform mat4 uReprojectionMatrix;
uniform uint osg_FrameNumber;

layout(std140, binding = 0) uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uInverseViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uInverseProjectionMatrix;
};

int bayerFilter4x4[] = {
    0, 8, 2, 10,
    12, 4, 14, 6,
    3, 11, 1, 9,
    15, 7, 13, 5
};

void main()
{
    ivec2 iTexcoord = ivec2(gl_FragCoord.xy) / 4;
    float dist = texelFetch(uCurrentCloudDistanceTexture, iTexcoord, 0).r * 1000.0;
    float depth;
    {
        float A = uProjectionMatrix[2][2], B = uProjectionMatrix[3][2];
        float near = B / (A - 1.0), far = B / (A + 1.0);
        depth = (-2.0 * near * far / dist + far + near) / (far - near);
    }
    uint bayerIndex = osg_FrameNumber % 16;
    ivec2 bayerOffset = ivec2(bayerFilter4x4[bayerIndex] % 4, bayerFilter4x4[bayerIndex] / 4);
    const ivec2 iDeltaCoord = ivec2(gl_FragCoord.xy) % 4;
    const bool needUpdate = (iDeltaCoord.x == bayerOffset.x) && (iDeltaCoord.y == bayerOffset.y);

    vec4 q = uReprojectionMatrix * vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec2 reprojectedUV = (q.xy * (1.0 / q.w)) * 0.5 + 0.5;
    bool isValidUV = reprojectedUV == clamp(reprojectedUV, vec2(0.0), vec2(1.0));

    vec4 color = texelFetch(uCurrentCloudColorTexture, iTexcoord, 0);
    vec4 history = textureLod(uHistoryCloudColorTexture, reprojectedUV, 0);

#if 1
    vec4 north = texelFetch(uCurrentCloudColorTexture, iTexcoord + ivec2(0, 1), 0);
    vec4 south = texelFetch(uCurrentCloudColorTexture, iTexcoord + ivec2(0, -1), 0);
    vec4 west = texelFetch(uCurrentCloudColorTexture, iTexcoord + ivec2(-1, 0), 0);
    vec4 east = texelFetch(uCurrentCloudColorTexture, iTexcoord + ivec2(1, 0), 0);
    vec4 boxMin = min(min(min(north, south), min(east, west)), color);
    vec4 boxMax = max(max(max(north, south), max(east, west)), color);
    history = clamp(history, boxMin, boxMax);
#endif

    vec4 rgba;
    float outDist;
    if (needUpdate || !isValidUV)
    {
        rgba = color;
        outDist = dist * 0.001;
    }
    else if (isValidUV)
    {
        rgba = history;//textureLod(uHistoryCloudColorTexture, reprojectedUV, 0);
        float historyDist = textureLod(uHistoryCloudDistanceTexture, reprojectedUV, 0).r * 1000.0;
        //float distDelta = abs(dist - historyDist);
        //if (distDelta > 1000.0 && distDelta < 10000.0)
        //    rgba = vec4(1, 0, 0, 0);
        if (history.a > 0.8 && length(history.rgb) > 0)
            rgba = vec4(1, 0, 0, 0);
        outDist = historyDist * 0.001;
    }
    else
    {
        rgba = textureLod(uCurrentCloudColorTexture, uv, 0);
        outDist = dist * 0.001;
    }
    fragData[0] = rgba;
    fragData[1] = vec4(outDist);
}