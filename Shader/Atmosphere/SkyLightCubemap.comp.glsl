#version 460 core
#extension GL_GOOGLE_include_directive : enable
layout(local_size_x = 32, local_size_y = 32) in;
layout(rgba16f, binding = 0) uniform imageCube uSkyLightCubemapImage;
uniform sampler2D uTransmittanceLutTexture;
uniform sampler2D uMultiScatteringLutTexture;

layout(std140, binding = 0) uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uInverseViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uInverseProjectionMatrix;
    mat4 uWorldToEnuMatrix;
    vec2 uNearFarPlane1;
    vec2 uNearFarPlane2;
    vec2 uTotalNearFarPlane;
};

#define SKY_VIEW_LUT
#define BINDING_INDEX 1
#include "AtmosphereCommon.glsl"
#define MAX_LUM 500.0

vec3 getSamplingVector()
{
    vec2 st = vec2(gl_GlobalInvocationID.xy + 0.5) / vec2(imageSize(uSkyLightCubemapImage));
    vec2 uv = 2.0 * vec2(st.x, 1.0 - st.y) - 1.0;
    vec3 dir[6] = {
		vec3(1.0, uv.y, -uv.x),
		vec3(-1.0, uv.y, uv.x),
		vec3(uv.x, 1.0, -uv.y),
		vec3(uv.x, -1.0, uv.y),
		vec3(uv.x, uv.y, 1.0),
		vec3(-uv.x, uv.y, -1.0)
	};
    return normalize(dir[gl_GlobalInvocationID.z]);
}

// z-up to y-up
vec3 worldToCubemap(vec3 dir)
{
    return vec3(dir.x, dir.z, -dir.y);
}

// y-up to z-up
vec3 cubemapToWorld(vec3 dir)
{
    return vec3(dir.x, -dir.z, dir.y);
}

void main()
{
    vec3 worldPos = uInverseViewMatrix[3].xyz / 1000.0;
    float viewHeight = length(worldPos);
    vec3 worldDir = transpose(mat3(uWorldToEnuMatrix)) * cubemapToWorld(getSamplingVector());
    float distanceToGround = rayIntersectSphere(worldPos, worldDir, uGroundRadius);
    vec3 groundNormal = normalize(worldPos + distanceToGround + worldDir);
    vec3 groundColor = uGroundAlbedo * step(0.0, distanceToGround) * max(dot(groundNormal, uSunDirection), 0.0);
    vec3 lum, fms, trans;
    rayMarchAtmosphere(0.3, worldPos, worldDir, uSunDirection, 30.0, 9000000.0, lum, fms, trans);
    imageStore(uSkyLightCubemapImage, ivec3(gl_GlobalInvocationID), vec4(lum * uSunIntensity + groundColor, 1.0));
}