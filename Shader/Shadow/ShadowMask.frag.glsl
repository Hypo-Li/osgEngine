#version 430 core
in vec2 uv;
out vec4 fragData;

layout(std140, binding = 0) uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uInverseViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uInverseProjectionMatrix;
};

const uint MAX_SHADOW_CASCADE_COUNT = 4;
layout(std140, binding = 1) uniform ShadowParameters
{
    mat4 uViewSpaceToLightSpaceMatrices[MAX_SHADOW_CASCADE_COUNT];
    // x: depth bias, y: slope bias, z: soft transition scale, w: cascade far
    vec4 uCascadeParameters[MAX_SHADOW_CASCADE_COUNT];
    float uReceiverBias;
};

uniform sampler2D uNormalTexture;
uniform sampler2D uDepthTexture;
uniform sampler2D uShadowMapTexture;
const vec4 gShadowMapResolution = vec4(8192.0, 2048.0, 0.0001220703125, 0.00048828125);
float gTransitionScale;
const vec3 lightDir = vec3(0.7071, 0.0, 0.7071);

vec4 calculateOcclusion(vec4 shadowmapDepth, float fragDepth)
{
    float constantFactor = fragDepth * gTransitionScale - 1.0;
    vec4 shadowFactor = clamp(shadowmapDepth * gTransitionScale - constantFactor, 0.0, 1.0);
    return shadowFactor;
}

vec2 horizontalPCF5x2(vec2 fraction, vec4 values00, vec4 values20, vec4 values40)
{
	float results0;
	float results1;

	results0 = values00.w * (1.0 - fraction.x);
	results1 = values00.x * (1.0 - fraction.x);
	results0 += values00.z;
	results1 += values00.y;
	results0 += values20.w;
	results1 += values20.x;
	results0 += values20.z;
	results1 += values20.y;
	results0 += values40.w;
	results1 += values40.x;
	results0 += values40.z * fraction.x;
	results1 += values40.y * fraction.x;

	return vec2(results0, results1);
}

float calculatePCF5x5(vec2 shadowMapUV, float fragDepth)
{
    vec2 texelPos = shadowMapUV * gShadowMapResolution.xy - 0.5;
	vec2 fraction = fract(texelPos);
	
	vec2 samplePos = (floor(texelPos) + 1.0) * gShadowMapResolution.zw;	

	vec4 values00 = calculateOcclusion(textureGatherOffset(uShadowMapTexture, samplePos, ivec2(-2, -2)), fragDepth);
	vec4 values20 = calculateOcclusion(textureGatherOffset(uShadowMapTexture, samplePos, ivec2( 0, -2)), fragDepth);
	vec4 values40 = calculateOcclusion(textureGatherOffset(uShadowMapTexture, samplePos, ivec2( 2, -2)), fragDepth);

	vec2 row0 = horizontalPCF5x2(fraction, values00, values20, values40);
	float results = row0.x * (1.0 - fraction.y) + row0.y;

	vec4 values02 = calculateOcclusion(textureGatherOffset(uShadowMapTexture, samplePos, ivec2(-2,  0)), fragDepth);
	vec4 values22 = calculateOcclusion(textureGatherOffset(uShadowMapTexture, samplePos, ivec2( 0,  0)), fragDepth);
	vec4 values42 = calculateOcclusion(textureGatherOffset(uShadowMapTexture, samplePos, ivec2( 2,  0)), fragDepth);

	vec2 row1 = horizontalPCF5x2(fraction, values02, values22, values42);
	results += row1.x + row1.y;

	vec4 values04 = calculateOcclusion(textureGatherOffset(uShadowMapTexture, samplePos, ivec2(-2,  2)), fragDepth);
	vec4 values24 = calculateOcclusion(textureGatherOffset(uShadowMapTexture, samplePos, ivec2( 0,  2)), fragDepth);
	vec4 values44 = calculateOcclusion(textureGatherOffset(uShadowMapTexture, samplePos, ivec2( 2,  2)), fragDepth);

	vec2 row2 = horizontalPCF5x2(fraction, values04, values24, values44);
	results += row2.x + row2.y * fraction.y;

	return 0.04 * results;
}

float PCF3x3gather(vec2 fraction, vec4 values0, vec4 values1, vec4 values2, vec4 values3)
{
    vec4 results;
    results.x = values0.w * (1.0 - fraction.x);
	results.y = values0.x * (1.0 - fraction.x);
	results.z = values2.w * (1.0 - fraction.x);
	results.w = values2.x * (1.0 - fraction.x);
	results.x += values0.z;
	results.y += values0.y;
	results.z += values2.z;
	results.w += values2.y;
	results.x += values1.w;
	results.y += values1.x;
	results.z += values3.w;
	results.w += values3.x;
	results.x += values1.z * fraction.x;
	results.y += values1.y * fraction.x;
	results.z += values3.z * fraction.x;
	results.w += values3.y * fraction.x;

	return dot(results, vec4( 1.0 - fraction.y, 1.0, 1.0, fraction.y) * ( 1.0 / 9.0));
}

float calculatePCF3x3(vec2 shadowMapUV, float fragDepth)
{
    vec2 texelPos = shadowMapUV * gShadowMapResolution.xy - 0.5;
	vec2 fraction = fract(texelPos);

	vec2 samplePos = (floor(texelPos) + 1.0) * gShadowMapResolution.zw;
    vec4 values0 = calculateOcclusion(textureGatherOffset(uShadowMapTexture, samplePos, ivec2(-1, -1)), fragDepth);
    vec4 values1 = calculateOcclusion(textureGatherOffset(uShadowMapTexture, samplePos, ivec2( 1, -1)), fragDepth);
    vec4 values2 = calculateOcclusion(textureGatherOffset(uShadowMapTexture, samplePos, ivec2(-1,  1)), fragDepth);
    vec4 values3 = calculateOcclusion(textureGatherOffset(uShadowMapTexture, samplePos, ivec2( 1,  1)), fragDepth);
    return PCF3x3gather(fraction, values0, values1, values2, values3);
}

float PCF1x1(vec2 fraction, vec4 values00)
{
    vec2 horizontalLerp00 = mix(values00.wx, values00.zy, fraction.xx);
    return mix(horizontalLerp00.x, horizontalLerp00.y, fraction.y);
}

float calculatePCF1x1(vec2 shadowMapUV, float fragDepth)
{
    vec2 texelPos = shadowMapUV * gShadowMapResolution.xy - 0.5;
	vec2 fraction = fract(texelPos);
    vec4 samples;
    vec2 quadCenter = floor(texelPos) + 1.0;
    samples = textureGather(uShadowMapTexture, quadCenter * gShadowMapResolution.zw);
    vec4 values00 = calculateOcclusion(samples, fragDepth);
    return PCF1x1(fraction, values00);
}

float calculateShadow(vec2 shadowMapUV, float fragDepth)
{
	return calculateOcclusion(textureLod(uShadowMapTexture, shadowMapUV, 0.0).rrrr, fragDepth).r;
}

float getShadow(vec4 fragPosVS, uint layer)
{
    vec4 lightSpace = uViewSpaceToLightSpaceMatrices[layer] * fragPosVS;
    lightSpace.xyz *= 1.0 / lightSpace.w;
    float fragDepth = lightSpace.z * 0.5 + 0.5;
    vec2 shadowMapUV = lightSpace.xy * 0.5 + 0.5;
    shadowMapUV = (shadowMapUV + vec2(float(layer), 0.0)) * vec2(0.25, 1.0);
    return calculateShadow(shadowMapUV, fragDepth);
}

const vec4 layerColor[4] = {
    vec4(1.0, 0.0, 0.0, 1.0),
    vec4(0.0, 1.0, 0.0, 1.0),
    vec4(0.0, 0.0, 1.0, 1.0),
    vec4(1.0, 1.0, 0.0, 1.0)
};

void main()
{
    float depth = texelFetch(uDepthTexture, ivec2(gl_FragCoord.xy), 0).r;
    if (depth == 1.0) return;
    vec4 fragPosVS = uInverseProjectionMatrix * vec4(vec3(uv, depth) * 2.0 - 1.0, 1.0);
    fragPosVS *= 1.0 / fragPosVS.w;
    float fragDistance = abs(fragPosVS.z);
    vec4 res = step(vec4(uCascadeParameters[0].w, uCascadeParameters[1].w, uCascadeParameters[2].w, uCascadeParameters[3].w), vec4(fragDistance));
    uint layer = uint(res.x + res.y + res.z + res.w);
    if (layer >= 4)
    {
        fragData = vec4(1.0);
        return;
    }
    vec3 normal = texelFetch(uNormalTexture, ivec2(gl_FragCoord.xy), 0).rgb;
    float ndl = dot(normal, lightDir);
    gTransitionScale = uCascadeParameters[layer].z * mix(uReceiverBias, 1.0, ndl);
    fragData = vec4(getShadow(fragPosVS, layer) * layerColor[layer]);
}