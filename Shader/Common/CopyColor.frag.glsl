#version 430 core
in vec2 uv;
out vec4 fragData;
uniform sampler2D uColorTexture;
void main()
{
    fragData = textureLod(uColorTexture, uv, 0);
}