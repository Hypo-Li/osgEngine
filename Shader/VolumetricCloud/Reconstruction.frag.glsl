#version 460 core
in vec2 uv;
out vec4 fragData;
uniform sampler2D uCurrentCloudColorTexture;
uniform sampler2D uCurrentDepthColorTexture;
uniform sampler2D uHistoryCloudColorTexture;
uniform sampler2D uHistoryCloudDepthTexture;
uniform mat4 uReprojectionMatrix;
uniform uint osg_FrameNumber;

int bayerFilter4x4[] = {
    0, 8, 2, 10,
    12, 4, 14, 6,
    3, 11, 1, 9,
    15, 7, 13, 5
};

void main()
{
    vec4 color = texelFetch(uCurrentCloudColorTexture, ivec2(gl_FragCoord.xy) / 4, 0);
    uint bayerIndex = osg_FrameNumber % 16;
    ivec2 bayerOffset = ivec2(bayerFilter4x4[bayerIndex] % 4, bayerFilter4x4[bayerIndex] / 4);
    ivec2 iDeltaCoord = ivec2(gl_FragCoord.xy) % 4;
    const bool needUpdate = (iDeltaCoord.x == bayerOffset.x) && (iDeltaCoord.y == bayerOffset.y);
    const vec2 curEvaluateCloudTexelSize = 1.0 / textureSize(uCurrentCloudColorTexture, 0);

    vec4 q = uReprojectionMatrix * vec4(uv * 2.0 - 1.0, 1.0, 1.0);
    vec2 reprojectedUV = (q.xy * (1.0 / q.w)) * 0.5 + 0.5;
    bool isValidUV = reprojectedUV == clamp(reprojectedUV, vec2(0.0), vec2(1.0));

    vec4 history = textureLod(uHistoryCloudColorTexture, reprojectedUV, 0);
    if (isValidUV)
    {
        if (needUpdate)
            fragData = mix(history, color, vec4(0.5));
        else
            fragData = history;//mix(history, color, vec4(0.05));
    }
    else
    {
        fragData = color;
    }

    //fragData = color;
}