#version 430 core
in vec2 uv;
out vec3 fragData;

uniform sampler2D uSceneDepthTexture;
uniform sampler2DArray uShadowMapTextureArray;
uniform vec4 uCascadeFar;

layout(std140, binding = 0) uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uInverseViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uInverseProjectionMatrix;
    mat4 uReprojectionMatrix;
};

void main()
{
    float fragDepth = texelFetch(uSceneDepthTexture, ivec2(gl_FragCoord.xy), 0).r;
    if (fragDepth == 1.0)
        discard;
    
    vec4 fragPosNdc = vec4(vec3(uv, fragDepth) * 2.0 - 1.0, 1.0);
    vec4 fragPosVS = uInverseProjectionMatrix * fragPosNdc;
    fragPosVS *= (1.0 / fragPosVS.w);
    float fragDist = abs(fragPosVS.z);
    vec4 res = step(uCascadeFar, vec4(fragDist));
    uint layer = min(uint(dot(res, vec4(1))), 3);
    const vec3 color[4] = {
        vec3(1, 0, 0),
        vec3(0, 1, 0),
        vec3(0, 0, 1),
        vec3(1, 1, 0)
    };
    fragData = color[layer];
}