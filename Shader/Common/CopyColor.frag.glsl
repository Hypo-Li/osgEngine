#version 430 core
in vec2 uv;
out vec4 fragData;
uniform sampler2D uColorTexture;

void main()
{
    fragData = texelFetch(uColorTexture, ivec2(gl_FragCoord.xy), 0);
}