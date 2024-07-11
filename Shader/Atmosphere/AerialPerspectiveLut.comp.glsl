#version 460 core
#extension GL_GOOGLE_include_directive : enable
layout (local_size_x = 16, local_size_y = 16, local_size_z = 4) in;
layout (rgba16f, binding = 0) uniform image3D uAerialPerspectiveLutImage;
uniform sampler2D uTransmittanceLutTexture;
uniform sampler2D uMultiScatteringLutTexture;
#define PLANET_TOP_AT_ABSOLUTE_WORLD_ORIGIN
layout(std140, binding = 0) uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uInverseViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uInverseProjectionMatrix;
};
#define AERIAL_PERSPECTIVE_LUT
#define BINDING_INDEX 1
#include "AtmosphereCommon.glsl"
#define LUT_WIDTH 32
#define LUT_HEIGHT 32

void main()
{
    vec2 uv = (gl_GlobalInvocationID.xy + 0.5) / vec2(LUT_WIDTH, LUT_HEIGHT);
    vec4 clipSpace = vec4(uv * 2.0 - 1.0, 0.5, 1.0);
    vec4 hViewPos = uInverseProjectionMatrix * clipSpace;
    vec3 worldDir = normalize(mat3(uInverseViewMatrix) * hViewPos.xyz / hViewPos.w);
    vec3 worldPos = getWorldPos(uInverseViewMatrix[3].xyz);
    
    float slice = ((float(gl_GlobalInvocationID.z) + 0.5) / SLICE_COUNT);
    slice *= slice * SLICE_COUNT;
    float tMax = aerialPerspectiveSliceToDepth(slice);
    // ray marching endpoint
    vec3 newPos = worldPos + tMax * worldDir;
    float viewHeight = length(newPos);
    // determine if the endpoint is underground
    if (viewHeight <= uGroundRadius + PLANET_RADIUS_OFFSET)
    {
        // move the endpoint above the ground
        newPos = normalize(newPos) * (uGroundRadius + PLANET_RADIUS_OFFSET + 0.01);
        worldDir = normalize(newPos - worldPos);
        tMax = length(newPos - worldPos);
    }
    float tMaxMax = tMax;
    viewHeight = length(worldPos);
    // determine whether the viewpoint is outside the atmosphere
    if (viewHeight >= uAtmosphereRadius)
    {
        vec3 prevWorldPos = worldPos;
        // move the viewpoint around the atmosphere and avoid inefficient stepping
        if (!moveToTopAtmosphere(worldPos, worldDir))
        {
            imageStore(uAerialPerspectiveLutImage, ivec3(gl_GlobalInvocationID.xyz), vec4(0.0, 0.0, 0.0, 0.0));
            return;
        }
        float lengthToAtmosphere = length(prevWorldPos - worldPos);
        // determines whether the maximum length of the step is less than the length of the move
        if (tMaxMax < lengthToAtmosphere)
        {
            imageStore(uAerialPerspectiveLutImage, ivec3(gl_GlobalInvocationID.xyz), vec4(0.0, 0.0, 0.0, 0.0));
            return;
        }
        // the maximum length of the corrected step is the length of the step actually taken in the atmosphere
        tMaxMax = max(0.0, tMaxMax - lengthToAtmosphere);
    }
    vec3 lum, fms, trans;
    rayMarchAtmosphere(0.3, worldPos, worldDir, uSunDirection, max(1.0, float(gl_GlobalInvocationID.z + 1.0) * 2.0), tMaxMax, lum, fms, trans);
    imageStore(uAerialPerspectiveLutImage, ivec3(gl_GlobalInvocationID.xyz), vec4(lum, 1.0 - dot(trans, vec3(0.333333))));
}