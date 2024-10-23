#if (PREFILTER_LEVEL == 0)
    #define LOCAL_SIZE_X 32
    #define LOCAL_SIZE_Y 32
    #define LOCAL_SIZE_Z 1
    #define SAMPLE_COUNT 1
#elif (PREFILTER_LEVEL == 1)
    #define LOCAL_SIZE_X 32
    #define LOCAL_SIZE_Y 32
    #define LOCAL_SIZE_Z 1
    #define SAMPLE_COUNT 16
#elif (PREFILTER_LEVEL == 2)
    #define LOCAL_SIZE_X 4
    #define LOCAL_SIZE_Y 4
    #define LOCAL_SIZE_Z 16
    #define SAMPLE_COUNT 256
#elif (PREFILTER_LEVEL == 3)
    #define LOCAL_SIZE_X 4
    #define LOCAL_SIZE_Y 4
    #define LOCAL_SIZE_Z 64
    #define SAMPLE_COUNT 1024
#elif (PREFILTER_LEVEL == 4)
    #define LOCAL_SIZE_X 4
    #define LOCAL_SIZE_Y 4
    #define LOCAL_SIZE_Z 64
    #define SAMPLE_COUNT 2048
#endif
#define CUBEMAP_FACE_SIZE 512

layout(local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y, local_size_z = LOCAL_SIZE_Z) in;
layout(binding = 0, rgba16f) uniform imageCube uPrefilterImage;
uniform samplerCube uCubemapTexture;
#define PI 3.14159265358

vec2 Hammersley(uint i, uint n)
{
    const float tof = 0.5 / float(0x80000000U);
    uint bits = i;
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return vec2(float(i)/float(n), float(bits) * tof);
}

float D_GGX(float NoH, float a2)
{
    float d = (NoH * NoH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

vec3 importanceSamplingGGX(vec2 u, float a)
{
    float phi = 2.0 * PI * u.x;
    float cosTheta2 = (1 - u.y) / (1 + (a + 1) * ((a - 1) * u.y));
    float cosTheta = sqrt(cosTheta2);
    float sinTheta = sqrt(1.0 - cosTheta2);
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

vec3 getSamplingVector()
{
    vec2 st = vec2(gl_GlobalInvocationID.xy + 0.5) / vec2(imageSize(uPrefilterImage));
    vec2 uv = 2.0 * vec2(st.x, 1.0 - st.y) - 1.0;
    vec3 dir[6] = {
		vec3(1.0, uv.y, -uv.x),
		vec3(-1.0, uv.y, uv.x),
		vec3(uv.x, 1.0, -uv.y),
		vec3(uv.x, -1.0, uv.y),
		vec3(uv.x, uv.y, 1.0),
		vec3(-uv.x, uv.y, -1.0)
	};
#if (PREFILTER_LEVEL < 2)
    return normalize(dir[gl_GlobalInvocationID.z]);
#else
    return normalize(dir[gl_WorkGroupID.z]);
#endif
}

float lodToPerceptualRoughness(float lod)
{
    const float a = 2.0;
    const float b = -1.0;
    return (lod != 0) ? clamp((sqrt(a * a + 4.0 * b * lod) - a) / (2.0 * b), 0.0, 1.0) : 0.0;
}

#if (PREFILTER_LEVEL >= 2)
shared vec4 temp[LOCAL_SIZE_X * LOCAL_SIZE_Y][LOCAL_SIZE_Z];
#endif

void main()
{
    const float lod = clamp(PREFILTER_LEVEL / 4.0, 0.0, 1.0);
    float roughness = lodToPerceptualRoughness(lod);
    float a = roughness * roughness;
    float a2 = a * a;

    vec3 N = getSamplingVector();
    vec3 T = normalize(cross(abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0), N));
    vec3 B = cross(N, T);
    mat3 TBN = mat3(T, B, N);

#if (PREFILTER_LEVEL < 2)
    const uint sampleCount = SAMPLE_COUNT;
    const uint offset = 0;
#else
    const uint sampleCount = SAMPLE_COUNT / LOCAL_SIZE_Z;
    const uint offset = gl_LocalInvocationID.z * sampleCount;
#endif
    vec3 prefilterdColor = vec3(0.0);
    float totalWeight = 0.0;
    for (uint i = 0; i < sampleCount; ++i)
    {
        vec2 Xi = Hammersley(i + offset, SAMPLE_COUNT);
        vec3 H = importanceSamplingGGX(Xi, a);
        float NoH = H.z;
        float NoL = 2.0 * NoH * NoH - 1.0;
        vec3 L = vec3(2.0 * NoH * H.xy, NoL);
        if (NoL > 0.0)
        {
            float pdf = D_GGX(NoH, a2) * 0.25;
            float omegaS = 1.0 / (SAMPLE_COUNT * pdf);
            const float omegaP = 4.0 * PI / (6.0 * CUBEMAP_FACE_SIZE * CUBEMAP_FACE_SIZE);
            float mipLevel = roughness == 0.0 ? 0.0 : clamp(0.5 * log2(omegaS / omegaP), 0.0, 4.0);
            prefilterdColor += textureLod(uCubemapTexture, TBN * L, mipLevel).rgb * NoL;
            totalWeight += NoL;
        }
    }
#if (PREFILTER_LEVEL < 2)
    imageStore(uPrefilterImage, ivec3(gl_GlobalInvocationID), vec4(prefilterdColor * (1.0 / totalWeight), 1.0));
#else
    const uint index = gl_LocalInvocationID.x * LOCAL_SIZE_X + gl_LocalInvocationID.y;
    temp[index][gl_LocalInvocationID.z] = vec4(prefilterdColor, totalWeight);
    barrier();
#if (PREFILTER_LEVEL >= 3)
    if (gl_LocalInvocationID.z < 32)
    {
        temp[index][gl_LocalInvocationID.z] += temp[index][gl_LocalInvocationID.z + 32];
    }
    barrier();
    if (gl_LocalInvocationID.z < 16)
    {
        temp[index][gl_LocalInvocationID.z] += temp[index][gl_LocalInvocationID.z + 16];
    }
    barrier();
#endif
    if (gl_LocalInvocationID.z < 8)
    {
        temp[index][gl_LocalInvocationID.z] += temp[index][gl_LocalInvocationID.z + 8];
    }
    barrier();
    if (gl_LocalInvocationID.z < 4)
    {
        temp[index][gl_LocalInvocationID.z] += temp[index][gl_LocalInvocationID.z + 4];
    }
    barrier();
    if (gl_LocalInvocationID.z < 2)
    {
        temp[index][gl_LocalInvocationID.z] += temp[index][gl_LocalInvocationID.z + 2];
    }
    barrier();
    if (gl_LocalInvocationID.z < 1)
    {
        vec4 colorWeight = temp[index][0] + temp[index][1];
        imageStore(uPrefilterImage, ivec3(gl_GlobalInvocationID.xy, gl_WorkGroupID.z), vec4(colorWeight.xyz * (1.0 / colorWeight.w), 1.0));
    }
#endif
}