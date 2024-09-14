#version 430 core
in vec2 uv;
out vec4 fragData[2];
uniform sampler2D uColorTexture;
uniform sampler2D uColor1Texture;

void main()
{
    fragData[0] = texelFetch(uColorTexture, ivec2(gl_FragCoord.xy), 0);
    fragData[1] = texelFetch(uColor1Texture, ivec2(gl_FragCoord.xy), 0);
}