#version 430 core
in vec2 uv;
out vec4 fragData;
uniform sampler3D uNoise1Texture;
uniform sampler3D uNoise2Texture;
uniform sampler2D uColorTexture;
uniform sampler2D uDepthTexture;
uniform float osg_FrameTime;
#define PI 3.1415926

layout(std140, binding = 0) uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uInverseViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uInverseProjectionMatrix;
};

float remap(float x, float a, float b, float c, float d)
{
    return (((x - a) / (b - a)) * (d - c)) + c;
}

float HenyeyGreenstein(float cosTheta, float g)
{
    float g2 = g * g;
    return ((1.0 - g2) / pow((1.0 + g2 - 2.0 * g * cosTheta), 1.5)) / (4.0 * PI);
}

vec2 intersectAABB(vec3 rayOrigin, vec3 rayDir, vec3 boxMin, vec3 boxMax)
{
    vec3 tMin = (boxMin - rayOrigin) / rayDir;
    vec3 tMax = (boxMax - rayOrigin) / rayDir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
	if (tNear > tFar)
		return vec2(-1, -1);
	tNear = max(tNear, 0.0);
	return vec2(tNear, tFar);
}

vec2 rayIntersectSphere(vec3 rayOrigin, vec3 rayDir, vec4 sphere)
{
    vec3 localPos = rayOrigin - sphere.xyz;
    float localPosSqr = dot(localPos, localPos);
    vec3 QuadraticCoef = vec3(
        dot(rayDir, rayDir),
        2 * dot(rayDir, localPos),
        localPosSqr - sphere.w * sphere.w
    );
    float discriminant = QuadraticCoef.y * QuadraticCoef.y - 4 * QuadraticCoef.x * QuadraticCoef.z;
    vec2 intersections = vec2(-1);
    if (discriminant >= 0)
    {
        float sqrtDiscriminant = sqrt(discriminant);
        intersections = (-QuadraticCoef.y + vec2(-1, 1) * sqrtDiscriminant) / (2 * QuadraticCoef.x);
    }
    return intersections;
}

float getDensity(vec3 p)
{
    vec3 uvw = p * 0.04;
    vec4 noise = textureLod(uNoise1Texture, uvw + mod(osg_FrameTime * 0.1, 1.0), 0.0);
    float wfbm = dot(noise.gba, vec3(0.625, 0.125, 0.25));
    float density = remap(noise.r, wfbm - 1.0, 1.0, 0.0, 1.0);
    return clamp(remap(density, 0.85, 1., 0., 1.), 0.0, 1.0);
}

#define CLOUD_STEPS 128
#define TRANS_STEPS 8

// Scattering and absorption coefficients
const vec3 sigmaS = vec3(1);
const vec3 sigmaA = vec3(0.0);
// Extinction coefficient.
const vec3 sigmaE = max(sigmaS + sigmaA, vec3(1e-6));

const vec3 sunIntensity = vec3(6.0);

vec3 rayMarchTransmittance(vec3 ro, vec3 rd, float dist)
{
    const float dt = dist / TRANS_STEPS;
    vec3 trans = vec3(1.0);
    vec3 p = ro;
    
    for (uint i = 0; i < TRANS_STEPS; ++i)
    {
        float density = getDensity(p);
        trans *= exp(-dt * density * sigmaE);
        p += rd * dt;
    }
    return trans;
}

vec4 rayMarchCloud(vec3 ro, vec3 rd, float dist, vec3 ld)
{
    const float dt = dist / CLOUD_STEPS;
    const float cosTheta = dot(rd, ld);
    const float phase = mix(HenyeyGreenstein(cosTheta, 0.5), HenyeyGreenstein(cosTheta, -0.3), 0.5);
    vec3 p = ro;
    vec3 color = vec3(0.0);
    vec3 trans = vec3(1.0);
    for (uint i = 0; i < CLOUD_STEPS; ++i)
    {
        float density = getDensity(p);
        vec3 sampleSigmaS = sigmaS * density;
        vec3 sampleSigmaE = sigmaE * density;

        if (density > 0.0)
        {   
            vec2 intersections = rayIntersectSphere(p, ld, vec4(0.0, 0.0, 0.0, 15.0));
            vec3 transToSun = rayMarchTransmittance(p, ld, intersections.y);
            vec3 luminance = sunIntensity * sampleSigmaS * phase * transToSun;
            vec3 sampleTrans = exp(-sampleSigmaE * dt);
            vec3 Sint = (luminance - luminance * sampleTrans) / sampleSigmaE;
            color += trans * Sint;
            trans *= sampleTrans;
            if (length(trans) <= 0.001)
                break;
        }
        p += dt * rd;
    }
    return vec4(color, trans);
}

vec3 aces(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
    vec3 color = pow(textureLod(uColorTexture, uv, 0.0).rgb, vec3(2.2));
    float depth = 1.0;//textureLod(uDepthTexture, uv, 0.0).r;

    vec4 viewSpace = uInverseProjectionMatrix * vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    viewSpace *= 1.0 / viewSpace.w;
    vec4 worldSpace = uInverseViewMatrix * viewSpace;
    vec3 cameraPosition = vec3(uInverseViewMatrix[3]);
    vec3 lookDirection = normalize(worldSpace.xyz - cameraPosition);

    vec2 intersections = rayIntersectSphere(cameraPosition, lookDirection, vec4(0.0, 0.0, 0.0, 15.0));
    float rayLength = length(worldSpace.xyz - cameraPosition);
    if (intersections.x < intersections.y && intersections.x < rayLength)
    {
		intersections.x = max(intersections.x, 0.0);
        intersections.y = min(intersections.y, rayLength);
        vec3 entryPoint = cameraPosition + intersections.x * lookDirection;
        vec4 cloudColor = rayMarchCloud(entryPoint, lookDirection, intersections.y - intersections.x, normalize(vec3(1.0, -1.0, 1.0)));
        color = color * cloudColor.a + cloudColor.rgb;
    }
    fragData = vec4(pow(aces(color), vec3(1.0/2.2)), 1.0);
}