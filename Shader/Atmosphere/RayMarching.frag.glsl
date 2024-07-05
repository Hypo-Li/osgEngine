#version 460 core
#extension GL_GOOGLE_include_directive : enable
in vec2 uv;
out vec4 fragData;
uniform sampler2D uTransmittanceLutTexture;
uniform sampler2D uMultiScatteringLutTexture;
uniform sampler2D uSkyViewLutTexture;
uniform sampler3D uAerialPerspectiveLutTexture;
uniform sampler2D uSceneDepthTexture;
uniform uint osg_FrameNumber;

#define PLANET_TOP_AT_ABSOLUTE_WORLD_ORIGIN
layout(std140, binding = 0) uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uInverseViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uInverseProjectionMatrix;
};
#define ATMOSPHERE_RAY_MARCHING
#define BINDING_INDEX 1
#include "AtmosphereCommon.glsl"
const float cosHalfApex = 0.99998;

float interleavedGradientNoise(vec2 uv, float frameId)
{
	uv += frameId * (vec2(47, 17) * 0.695);
    const vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic.z * fract(dot(uv, magic.xy)));
}

void main()
{
    vec4 clipSpace = vec4(uv * 2.0 - 1.0, 1.0, 1.0);
    vec4 viewSpace = uInverseProjectionMatrix * clipSpace;
    //viewSpace *= 1.0 / viewSpace.w;
    vec3 worldDir = normalize(mat3(uInverseViewMatrix) * viewSpace.xyz);
    vec3 worldPos = getWorldPos(uInverseViewMatrix[3].xyz);
    float viewHeight = length(worldPos);
    float sceneDepth = texture(uSceneDepthTexture, uv).r;
    bool intersectGround = rayIntersectSphere(worldPos, worldDir, uGroundRadius) >= 0.0;

    vec3 outLuminance = vec3(0.0);
    if (sceneDepth == 1.0)
    {
        float vdl = dot(worldDir, uSunDirection); // world space vdl
        vec3 upVector = worldPos / viewHeight;

        if (!intersectGround && vdl > cosHalfApex/*cos(SUN_DISK_RADIUS * SUN_DISK_RADIUS * PI / 180.0)*/)
        {
            float viewZenithCos = max(dot(uSunDirection, upVector), 0.0);
            vec2 transmittanceLutUV;
            transmittanceLutParametersToUV(viewHeight, viewZenithCos, transmittanceLutUV);
            vec3 trans = texture(uTransmittanceLutTexture, transmittanceLutUV).rgb;
            float softEdge = clamp(2.0 * (vdl - cosHalfApex) / (1.0 - cosHalfApex), 0.0, 1.0);
            outLuminance += trans * uSunIntensity * 1000.0 * softEdge;
        }

        if (viewHeight < uAtmosphereRadius)
        {
            float viewZenithCos = dot(worldDir, upVector);
            vec3 sideVector = normalize(cross(upVector, worldDir));
            vec3 forwardVector = normalize(cross(sideVector, upVector));
            vec2 lightOnPlane = vec2(dot(uSunDirection, forwardVector), dot(uSunDirection, sideVector));
            lightOnPlane = normalize(lightOnPlane);
            float lightViewCos = lightOnPlane.x;
    
            vec2 skyViewUV;
            skyViewLutParametersToUV(intersectGround, viewZenithCos, lightViewCos, max(viewHeight, uGroundRadius + 0.005), skyViewUV);
            outLuminance += texture(uSkyViewLutTexture, skyViewUV).rgb * uSunIntensity;
            fragData = vec4(outLuminance, 1.0);
            return;
        }
    }

    if (!moveToTopAtmosphere(worldPos, worldDir))
    {
        fragData = vec4(outLuminance, 1.0);
        return;
    }

    clipSpace = vec4(uv * 2.0 - 1.0, sceneDepth * 2.0 - 1.0, 1.0);
    viewSpace = uInverseProjectionMatrix * clipSpace;
    viewSpace *= 1.0 / viewSpace.w;
    float tDepth = abs(viewSpace.z) / 1000.0;

    if ((viewHeight - uGroundRadius) < (uAtmosphereRadius - uGroundRadius) * 0.05)
    {
        float slice = aerialPerspectiveDepthToSlice(tDepth);
        float weight = 1.0;
        if (slice < 0.5)
        {
            weight = clamp(slice * 2.0, 0.0, 1.0);
            slice = 0.5;
        }
        float w = sqrt(slice / SLICE_COUNT);
        vec4 AP = weight * texture(uAerialPerspectiveLutTexture, vec3(uv, w));
        fragData = vec4(AP.rgb * uSunIntensity, 1.0 - AP.a);
    }
    else
    {
        vec3 lum, fms, trans;
        rayMarchAtmosphere(interleavedGradientNoise(gl_FragCoord.xy, osg_FrameNumber % 8), worldPos, worldDir, uSunDirection, 0.0, 9000000, sceneDepth, tDepth, lum, fms, trans);
        fragData = vec4(lum * uSunIntensity, dot(trans, vec3(0.3333333)));
    }
}