#version 430 core
in vec2 uv;
out vec4 fragData;
uniform sampler2D uColorTexture;
uniform sampler2D uShadowMaskTexture;

void main()
{
    vec3 color = texelFetch(uColorTexture, ivec2(gl_FragCoord.xy), 0).rgb;
    vec3 shadow = texelFetch(uShadowMaskTexture, ivec2(gl_FragCoord.xy), 0).rgb;
    fragData = vec4(color * shadow, 1.0);
}