#version 430 core
in vec2 uv;
out vec4 fragData;
uniform vec4 uResolution;

void main()
{
    fragData = vec4(abs(gl_FragCoord.xy * uResolution.zw - uv), 0.0, 1.0);
}