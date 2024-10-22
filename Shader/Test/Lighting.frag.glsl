//#version 430 core
in vec2 texcoord;
out vec4 fragData;
uniform sampler2D uNormalTexture;
uniform sampler2D uBaseColorTexture;
uniform sampler2D uMetallicRoughnessTexture;
uniform sampler2D uDepthTexture;
uniform sampler2D uShadowMaskTexture;
uniform sampler2D uSSAOTexture;
uniform sampler2D uTransmittanceLutTexture;
uniform sampler2D uBRDFLutTexture;
uniform samplerCube uPrefilterTexture;
uniform float uAmbientDiffuseFactor;
uniform float uAmbientSpecularFactor;
uniform float uShadowStrength;

layout(std140, binding = 0) buffer SphericalHarmonicsCoefficient
{
    vec3 SHCoeff[9];
};

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

layout(std140, binding = 1) uniform AtmosphereParameters
{
    vec3 uRayleighScatteringBase;
    float uMieScatteringBase;
    vec3 uOzoneAbsorptionBase;
    float uMieAbsorptionBase;
    float uRayleighDensityH;
    float uMieDensityH;
    float uOzoneHeight;
    float uOzoneThickness;
    vec3 uGroundAlbedo;
    float uGroundRadius;
    vec3 uSunDirection;
    float uAtmosphereRadius;
    vec3 uSunIntensity;
};

const float PI = 3.14159265358979323846;
const float TAU = 2.0 * PI;

vec3 srgb_to_linear(vec3 srgb) { return pow(srgb, vec3(2.2)); } 

// z-up to y-up
vec3 worldToCubemap(vec3 dir)
{
    return vec3(dir.x, dir.z, -dir.y);
}

vec3 getSphericalHarmonicsLighting(vec3 n)
{
    vec3 result = vec3(0.0);
    result += SHCoeff[0] * 0.5 * sqrt(1.0 / PI);
    result -= SHCoeff[1] * 0.5 * sqrt(3.0 / PI) * n.y;
    result += SHCoeff[2] * 0.5 * sqrt(3.0 / PI) * n.z;
    result -= SHCoeff[3] * 0.5 * sqrt(3.0 / PI) * n.x;
    result += SHCoeff[4] * 0.5 * sqrt(15.0 / PI) * n.x * n.y;
    result -= SHCoeff[5] * 0.5 * sqrt(15.0 / PI) * n.y * n.z;
    result += SHCoeff[6] * 0.25 * sqrt(5.0 / PI) * (3.0 * n.z * n.z - 1.0);
    result -= SHCoeff[7] * 0.5 * sqrt(15.0 / PI) * n.x * n.z;
    result += SHCoeff[8] * 0.25 * sqrt(15.0 / PI) * (n.x * n.x - n.y * n.y);
    return result;
}

void transmittanceLutParametersToUV(in float viewHeight, in float viewZenithCos, out vec2 uv)
{
    float H = sqrt(max(0.0f, uAtmosphereRadius * uAtmosphereRadius - uGroundRadius * uGroundRadius));
    float rho = sqrt(max(0.0f, viewHeight * viewHeight - uGroundRadius * uGroundRadius));
    float discriminant = viewHeight * viewHeight * (viewZenithCos * viewZenithCos - 1.0) + uAtmosphereRadius * uAtmosphereRadius;
    float d = max(0.0, (-viewHeight * viewZenithCos + sqrt(discriminant))); // Distance to atmosphere boundary
    float dMin = uAtmosphereRadius - viewHeight;
    float dMax = rho + H;
    float u = (d - dMin) / (dMax - dMin);
    float v = rho / H;
    uv = vec2(u, v); 
}

vec3 F_Schlick(float vdh, vec3 F0)
{
    // 1.0 - vdh maybe equal to -0.0, pow will return nan
    return F0 + (1.0 - F0) * pow(abs(1.0 - vdh), 5.0);
}

vec3 F_Schlick(float vdh, vec3 F0, float F90)
{
    // 1.0 - vdh maybe equal to -0.0, pow will return nan
    return F0 + (F90 - F0) * pow(abs(1.0 - vdh), 5.0);
}

float D_GGX(float ndh, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float d = (ndh * ndh) * (a2 - 1.0) + 1.0;
    return clamp(a2 / (PI * d * d), 0.0, 1000.0);
}

float V_SmithGGXCorrelated(float ndv, float ndl, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float lambdaV = ndl * sqrt((ndv - a2 * ndv) * ndv + a2);
    float lambdaL = ndv * sqrt((ndl - a2 * ndl) * ndl + a2);
    return clamp(0.5 / (lambdaV + lambdaL), 0.0, 1.0);
}

float V_SmithGGXCorrelated_Fast(float ndv, float ndl, float roughness)
{
    return clamp(0.5 / mix(2.0 * ndl * ndv, ndl + ndv, roughness * roughness), 0.0, 1.0);
}

float computeSpecularAO(float ndv, float ao, float a)
{
    return clamp(pow(ndv + ao, exp2(-16.0 * a - 1.0)) - 1.0 + ao, 0.0, 1.0);
}

void main()
{
    ivec2 iTexcoord = ivec2(gl_FragCoord.xy);
    float depth = texelFetch(uDepthTexture, iTexcoord, 0).r;
    if (depth == 1.0) discard;
    vec3 normal =  normalize(texelFetch(uNormalTexture, iTexcoord, 0).rgb);
    vec3 baseColor = srgb_to_linear(texelFetch(uBaseColorTexture, iTexcoord, 0).rgb);
    vec2 metallicRoughness = texelFetch(uMetallicRoughnessTexture, iTexcoord, 0).rg;
    float shadow = 1.0 - texelFetch(uShadowMaskTexture, iTexcoord, 0).r;
    //vec3 shadow = texelFetch(uShadowMaskTexture, iTexcoord, 0).rgb;
    float ssao = textureLod(uSSAOTexture, texcoord, 0).r;

    vec4 fragPos = uInverseProjectionMatrix * vec4(texcoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    fragPos.xyz *= 1.0 / fragPos.w;
    float metallic = metallicRoughness.x;
    float roughness = max(metallicRoughness.y, 0.05);
    float a = roughness * roughness;
    float a2 = a * a;

    vec3 viewDir = normalize(-fragPos.xyz);
    vec3 lightDir = normalize(mat3(uViewMatrix) * uSunDirection);
    vec3 halfDir = normalize(lightDir + viewDir);
    vec3 reflectDir = reflect(-viewDir, normal);
    reflectDir = mix(reflectDir, normal, a2);
    float ndv = max(dot(normal, viewDir), 0.0);
    float ndl = max(dot(normal, lightDir), 0.0);
    float ndh = max(dot(normal, halfDir), 0.0);
    float vdh = max(dot(viewDir, halfDir), 0.0);
    vec3 F0 = mix(vec3(0.04), baseColor, metallic);

    vec3 trans;
    vec3 worldFragPos = vec3(uInverseViewMatrix * vec4(fragPos.xyz, 1.0)) / 1000.0;
    float viewHeight = length(worldFragPos);
    vec3 upVector = normalize(uInverseViewMatrix[3].xyz);
    float viewZenithCos = max(dot(uSunDirection, upVector), 0.0);
    vec2 transmittanceLutUV;
    transmittanceLutParametersToUV(viewHeight, viewZenithCos, transmittanceLutUV);
    trans = texture(uTransmittanceLutTexture, transmittanceLutUV).rgb;

    vec3 F = F_Schlick(vdh, F0);
    float D = D_GGX(ndh, roughness);
    float V = V_SmithGGXCorrelated(ndv, ndl, roughness);
    vec3 directDiffuse = baseColor * (1.0 - F) * (1.0 - metallic) / PI;
    vec3 directSpecular = F * D * V;
    vec3 direct = (directDiffuse + directSpecular) * ndl * uSunIntensity * trans * max(sign(viewZenithCos), 0);

    vec3 enuNormal = worldToCubemap(mat3(uWorldToEnuMatrix) * mat3(uInverseViewMatrix) * normal);
    vec3 enuReflectDir = worldToCubemap(mat3(uWorldToEnuMatrix) * mat3(uInverseViewMatrix) * reflectDir);
    vec2 brdf = texture(uBRDFLutTexture, vec2(ndv, roughness)).rg;
    vec3 E = mix(brdf.xxx, brdf.yyy, F0);
    vec3 indirectDiffuse = getSphericalHarmonicsLighting(enuNormal);
    indirectDiffuse *= baseColor * (1.0 - E) * (1.0 - metallic) * ssao;
    vec3 prefilter = textureLod(uPrefilterTexture, enuReflectDir, roughness * (2.0 - roughness) * 4.0).rgb;
    vec3 energyCompensation = 1.0 + F0 * (1.0 / brdf.y - 1.0);
    vec3 indirectSpecular = prefilter * E * energyCompensation * computeSpecularAO(ndv, ssao, a);
    vec3 indirect = indirectDiffuse * uAmbientDiffuseFactor + indirectSpecular * uAmbientSpecularFactor * clamp(dot(uSunDirection, upVector) + 0.07, 0.0, 1.0);

    vec3 lum = step(viewZenithCos, 0.0) == 1.0 ? /*night*/(directDiffuse + indirectSpecular) * vec3(0.84, 0.9255, 0.9412) * 0.05 : /*day*/direct * shadow + indirect;

    fragData = vec4(lum, 1.0 - smoothstep(0.0, 0.07, viewZenithCos));
}