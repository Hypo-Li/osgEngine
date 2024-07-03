#version 460 core
const float PI = 3.1415926535897932;

vec2 rayIntersectSphere(vec3 rayOrigin, vec3 rayDirection, vec4 sphere)
{
    vec3 localPosition = rayOrigin - sphere.xyz;
	float localPositionSqr = dot(localPosition, localPosition);

	vec3 quadraticCoef;
	quadraticCoef.x = dot(rayDirection, rayDirection);
	quadraticCoef.y = 2 * dot(rayDirection, localPosition);
	quadraticCoef.z = localPositionSqr - sphere.w * sphere.w;

	float discriminant = quadraticCoef.y * quadraticCoef.y - 4 * quadraticCoef.x * quadraticCoef.z;
	vec2 intersections = vec2(-1);
	if (discriminant >= 0)
	{
		float sqrtDiscriminant = sqrt(discriminant);
		intersections = (-quadraticCoef.y + vec2(-1, 1) * sqrtDiscriminant) / (2 * quadraticCoef.x);
	}
	return intersections;
}

float henyeyGreensteinPhase(float cosTheta, float g)
{
    float numer = 1.0 - g * g;
    float denom = 1.0 + g * g + 2.0 * g * cosTheta;
    return numer / (4.0 * PI * denom * sqrt(denom));
}

float rayleighPhase(float cosTheta)
{
    float factor = 3.0 / (16.0 * PI);
    return factor * (1.0 + cosTheta * cosTheta);
}

#define ATMOSPHERE_PARAMETERS_BINDING 1
layout (std140, binding = ATMOSPHERE_PARAMETERS_BINDING) uniform AtmosphereParameters
{
    vec3 uRayleighScatteringBase;
    float uMieScatteringBase;
    vec3 uOzoneAbsorptionBase;
    float uMieAbsorptionBase;
    float uRayleighDensityH;
    float uMieDensityH;
    float uOzoneCenterHeight;
    float uOzoneThickness;
    vec3 uGroundAlbedo;
    float uGroundRadius;
    vec3 uSunDirection;
    float uAtmosphereRadius;
    vec3 uSunIntensity;
};

struct AtmosphereMediumSampling
{
    vec3 rayleighScattering;
    float mieScattering;
    vec3 scattering;
    vec3 extinction;
};