#version 430 core
in vec2 uv;
out vec4 fragData;
uniform sampler2DMS uColorMsTexture;

void main()
{
    ivec2 iTexcoord = ivec2(gl_FragCoord.xy);
    vec4 color[4];
    color[0] = texelFetch(uColorMsTexture, iTexcoord, 0);
    color[1] = texelFetch(uColorMsTexture, iTexcoord, 1);
    color[2] = texelFetch(uColorMsTexture, iTexcoord, 2);
    color[3] = texelFetch(uColorMsTexture, iTexcoord, 3);

    fragData = (color[0] + color[1] + color[2] + color[3]) * 0.25;
}