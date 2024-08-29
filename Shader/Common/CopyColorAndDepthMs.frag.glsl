#version 430 core
in vec2 uv;
out vec4 fragData;
uniform sampler2DMS uColorMsTexture;
uniform sampler2DMS uDepthMsTexture;

void main()
{
    ivec2 iTexcoord = ivec2(gl_FragCoord.xy);
    vec4 color[4];
    color[0] = texelFetch(uColorMsTexture, iTexcoord, 0);
    color[1] = texelFetch(uColorMsTexture, iTexcoord, 1);
    color[2] = texelFetch(uColorMsTexture, iTexcoord, 2);
    color[3] = texelFetch(uColorMsTexture, iTexcoord, 3);
    float depth[4];
    depth[0] = texelFetch(uDepthMsTexture, iTexcoord, 0).r;
    depth[1] = texelFetch(uDepthMsTexture, iTexcoord, 1).r;
    depth[2] = texelFetch(uDepthMsTexture, iTexcoord, 2).r;
    depth[3] = texelFetch(uDepthMsTexture, iTexcoord, 3).r;

    fragData = (color[0] + color[1] + color[2] + color[3]) * 0.25;
    gl_FragDepth = min(min(depth[0], depth[1]), min(depth[2], depth[3]));
}