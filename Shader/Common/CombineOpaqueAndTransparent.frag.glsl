#version 430 core
in vec2 uv;
out vec4 fragData;
uniform sampler2D uOpaqueColorTexture;
uniform sampler2DMS uTransparentColorTexture;

void main()
{
    ivec2 iTexcoord = ivec2(gl_FragCoord.xy);
    vec4 opaqueColor = texelFetch(uOpaqueColorTexture, iTexcoord, 0);
    vec4 transparentColorMs0 = texelFetch(uTransparentColorTexture, iTexcoord, 0);
    vec4 transparentColorMs1 = texelFetch(uTransparentColorTexture, iTexcoord, 1);
    vec4 transparentColorMs2 = texelFetch(uTransparentColorTexture, iTexcoord, 2);
    vec4 transparentColorMs3 = texelFetch(uTransparentColorTexture, iTexcoord, 3);
    vec3 blendedColor0 = mix(transparentColorMs0.rgb, opaqueColor.rgb, transparentColorMs0.a);
    vec3 blendedColor1 = mix(transparentColorMs1.rgb, opaqueColor.rgb, transparentColorMs1.a);
    vec3 blendedColor2 = mix(transparentColorMs2.rgb, opaqueColor.rgb, transparentColorMs2.a);
    vec3 blendedColor3 = mix(transparentColorMs3.rgb, opaqueColor.rgb, transparentColorMs3.a);
    fragData = vec4((blendedColor0 + blendedColor1 + blendedColor2 + blendedColor3) * 0.25, 1.0);
}