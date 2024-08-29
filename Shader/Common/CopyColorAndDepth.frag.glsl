#version 430 core
in vec2 uv;
out vec4 fragData;
uniform sampler2D uColorTexture;
uniform sampler2D uDepthTexture;

void main()
{
    fragData = textureLod(uColorTexture, uv, 0.0);
    gl_FragDepth = textureLod(uDepthTexture, uv, 0.0).r;
}