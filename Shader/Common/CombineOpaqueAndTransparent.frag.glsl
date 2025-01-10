#version 430 core
in vec2 uv;
out vec4 fragData;
uniform sampler2D uOpaqueColorTexture;
uniform sampler2D uTransparentColorTexture;

void main()
{
    ivec2 iTexcoord = ivec2(gl_FragCoord.xy);
    vec4 opaqueColor = texelFetch(uOpaqueColorTexture, iTexcoord, 0);
    vec4 transparentColor = texelFetch(uTransparentColorTexture, iTexcoord, 0);
    fragData = vec4(mix(transparentColor.rgb, opaqueColor.rgb, transparentColor.a), transparentColor.a);
}