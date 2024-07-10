#version 460 core
#extension GL_GOOGLE_include_directive : enable
#include "intersection.glsl"
in vec2 uv;
out vec4 fragData;
uniform sampler2D uCloudMapTexture;
uniform sampler3D uBasicNoiseTexture;
uniform sampler3D uDetailNoiseTexture;
uniform sampler2D uTransmittanceLutTexture;
uniform sampler2D uMultiScatteringLutTexture;
uniform sampler2D uSkyViewLutTexture;
uniform sampler3D uAerialPerspectiveLutTexture;
uniform float osg_FrameTime;

layout(std140, binding = 0) uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uInverseViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uInverseProjectionMatrix;
};

layout (std140, binding = 1) uniform AtmosphereParameters
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

const float PI = 3.1415926535897932;

const float cloudBottomRadius = 6370.0;
const float cloudThickness = 10.0;
const float cloudTopRadius = cloudBottomRadius + cloudThickness;
const float cloudTracingStartMaxDistance = 50.0;
const float cloudMarchingStepCount = 128;
const vec3 windDirection = vec3(0.8728715181, 0.2182178795, 0.4364357591);
const float windSpeed = 0.1;

float saturate(float x) { return clamp(x, 0.0, 1.0); }
vec3 saturate(vec3 x) { return clamp(x, vec3(0.0), vec3(1.0)); }

float remap(float value, float oldMin, float oldMax, float newMin, float newMax)
{
    return newMin + (saturate((value - oldMin) / (oldMax - oldMin)) * (newMax - newMin));
}

#define PLANET_TOP_AT_ABSOLUTE_WORLD_ORIGIN
vec3 getWorldPos(vec3 pos)
{
#ifdef PLANET_TOP_AT_ABSOLUTE_WORLD_ORIGIN
    vec3 worldPos = pos / 1000.0 + vec3(0.0, 0.0, uGroundRadius);
#else
    vec3 worldPos = pos / 1000.0;
#endif
    float viewHeight = length(worldPos);
    vec3 upVector = worldPos / viewHeight;
    return upVector * max(viewHeight, uGroundRadius + 0.005);
}

// See http://www.pbr-book.org/3ed-2018/Volume_Scattering/Phase_Functions.html
float hgPhase(float g, float cosTheta)
{
	float numer = 1.0 - g * g;
	float denom = 1.0 + g * g + 2.0 * g * cosTheta;
	return numer / (4.0 * PI * denom * sqrt(denom));
}

float dualLobPhase(float g0, float g1, float w, float cosTheta)
{
	return mix(hgPhase(g0, cosTheta), hgPhase(g1, cosTheta), w);
}

float GetDirectScatterProbability(float eccentricity, float cosTheta)
{
    const float u_SilverIntensity = 1.0;
    const float u_SilverSpread = 0.5;
    return max(hgPhase(eccentricity, cosTheta), u_SilverIntensity * hgPhase(0.99 - u_SilverIntensity, cosTheta));
}

float GetAttenuationProbability(float sampleDensity)
{
    return max(exp(-sampleDensity), (exp(-sampleDensity * 0.25) * 0.7));
}

float GetInScatterProbability(vec3 p, float ds_loded, float height_fraction)
{
    // 计算深度密度函数
    float depth_probability = 0.05 + pow(ds_loded, remap(height_fraction, 0.3, 0.85, 0.5, 2.0));

    // 计算垂向分布
    float vertical_probability = pow(remap(height_fraction, 0.07, 0.14, 0.1, 1.0 ), 0.8 );

    // 最终的内散射计算
    float in_scatter_probability = depth_probability * vertical_probability;

    return in_scatter_probability;

}

vec3 GetLightEnergy(vec3 p, float dl, float ds_loded, float cos_angle, float height_fraction)
{
    // 吸收模型
    float attenuation_probability = GetAttenuationProbability(dl);

    // 内散射模型
    float in_scatter_probability = GetInScatterProbability(p, ds_loded, height_fraction);

    // 定向散射模型
    const float eccentricity = 0.8;
    float phase_probability = GetDirectScatterProbability(eccentricity, cos_angle);
        
    // 最终的光照是方向散射，吸收，内散射相乘，再乘上平均亮度
    float light_energy = attenuation_probability * in_scatter_probability * phase_probability;

    return light_energy * uSunIntensity;
}

float GetHeightFractionForPoint( vec3 inPosition, vec2 inCloudMinMax )
{
    float height_fraction = (inPosition.z - inCloudMinMax.x) / (inCloudMinMax.y - inCloudMinMax.x);
    return saturate(height_fraction);
}

float GetDensityHeightGradientForPoint(in float RelativeHeight, in float CloudType)
{
    //CloudType = remap(CloudType, u_CloudTypeOffset, 1, 0, 1);
    RelativeHeight = clamp(RelativeHeight, 0.0,1.0);

    // 根据2017年的分享，两个Remap相乘重建云属
    float Cumulus = max(0.0, remap(RelativeHeight, 0.01, 0.3, 0.0, 1.0) * remap(RelativeHeight, 0.6, 0.95, 1.0, 0.0));
    float Stratocumulus = max(0.0, remap(RelativeHeight, 0.0, 0.25, 0.0, 1.0) * remap(RelativeHeight, 0.3, 0.65, 1.0, 0.0));
    float Stratus = max(0.0, remap(RelativeHeight, 0, 0.1, 0.0, 1.0) * remap(RelativeHeight, 0.2, 0.3, 1.0, 0.0));

    // 云属过渡
    float a = mix(Stratus, Stratocumulus, clamp(CloudType * 2.0, 0.0, 1.0));
    float b = mix(Stratocumulus, Cumulus, clamp((CloudType - 0.5) * 2.0, 0.0, 1.0));
    return mix(a, b, CloudType);
}

float SampleLowFrequencyNoises(vec3 p, float mip_level)
{
    const float basicNoiseScale = 1.0;

    vec4 low_frequency_noises = textureLod(uBasicNoiseTexture, p * basicNoiseScale, mip_level);

    // 从低频 Worley 噪声中构建一个 fBm，可用于为低频 Perlin-Worley 噪声添加细节
    // 这主要用于改善连贯的Perlin-worley噪声，使其产生孤立的岛状云
    float low_freq_fBm = ( low_frequency_noises.g * 0.625 ) + ( low_frequency_noises.b * 0.25 ) + ( low_frequency_noises.a * 0.125 );

    // 通过使用由 Worley 噪声构成的低频 FBM 对其进行膨胀来定义基本云形状。
    float base_cloud = remap( low_frequency_noises.r, - ( 1.0 -  low_freq_fBm), 1.0, 0.0, 1.0 );

    return base_cloud;
}

float SampleHighFrequencyNoises(vec3 p, float mip_level)
{
    // 参考SIG 2015中的做法，使用Curl Noise为云增加一些湍流感
    //float2 curl_noise = tex2Dlod(tCloud2DNoiseTexture,  sCloud2DNoiseSampler,  float4 (float2(p.x, p.y), 0.0, 1.0).rg;
    //p.xy += curl_noise.rg * (1.0 - height_fraction) * 200.0;

    const float detailNoiseScale = 0.1;

    // 采样高频噪声
    vec3 high_frequency_noises = textureLod(uDetailNoiseTexture,  p * detailNoiseScale, mip_level).rgb;

    // 构建高频worley噪声FBM
    float high_freq_fBm = ( high_frequency_noises.r * 0.625 ) + ( high_frequency_noises.g * 0.25 ) + ( high_frequency_noises.b * 0.125 );

    return high_freq_fBm;
}

vec3 SampleWeather(vec3 pos)
{
    //vec2 uv = (pos.xy-Coverage.xy) / u_Coverage.zw;
    //uv += CoverageUVOffset;
    vec2 uv = pos.xy * 0.006;
    
    vec3 weatherData = textureLod(uCloudMapTexture, uv, 0).rgb;
    return weatherData;
}

float SampleCloudDensity(vec3 p, vec3 weather_data, int mipLevel, bool doCheaply)
{
    float height_fraction = GetHeightFractionForPoint(p, vec2(cloudBottomRadius, cloudTopRadius));
    
    vec3 wind_direction = windDirection;
    float cloud_speed = windSpeed; // PPT中给的是10

    // cloud_top offset ，用于偏移高处受风力影响的程度
    float cloud_top_offset = 500.0;

    // 为采样位置增加高度梯度，风向影响;
    p += height_fraction * wind_direction * cloud_top_offset;

    // 增加一些沿着风向的时间偏移
    p += (wind_direction + vec3(0.0, 0.1, 0.0)  ) * osg_FrameTime * cloud_speed;

    // 这个函数封装了Perlin-Worley Noise与3层频率递增Worley Noise的采样与FBM；
    float base_cloud = SampleLowFrequencyNoises(p, mipLevel);
    
    // 依据高度梯度模型计算云属，按照SIG 2015的设定，云图的B通道存储了CloudType
    float density_height_gradient = GetDensityHeightGradientForPoint(height_fraction, weather_data.b);

    // 现在已经可以通过高度-密度模型塑造云的大型了
    float noise = base_cloud * density_height_gradient;

    // 按照原文设定，云图的R通道存储了天穹内的覆盖范围，随着Coverage逐渐增大天穹逐渐被云层覆盖
    float coverage = weather_data.r;

    // 这次Remap用于将云向高度方向拉伸以模拟2015年失败的“积雨云”，它们有着垂直的塔状分布
    // 这虽然没“铁砧”形自然，但看去来很酷
    const float u_AnvilBias = 0.5;
    float cloud_coverage = pow(coverage, remap(height_fraction, 0.7, 0.8, 1.0, mix(1.0, 0.5, u_AnvilBias)));
    
    // 对最终的noise与Coverage做Remap,也可以在PPT中找到对应说明
    float cloud_with_coverage  = remap(noise, cloud_coverage, 1.0, 0.0, 1.0); 

    cloud_with_coverage *= cloud_coverage;
    float final_cloud = cloud_with_coverage;

    if(!doCheaply)
    {   
        // 采样高频噪声并构建FBM
        float high_freq_fBm = SampleHighFrequencyNoises(p, mipLevel);

        // 获取height_fraction用于在高度上混合噪声
        float height_fraction  = GetHeightFractionForPoint(p, vec2(cloudBottomRadius, cloudTopRadius));

        // 依据高度从纤细的形状过渡到波浪形状
        float high_freq_noise_modifier = mix(high_freq_fBm, 1.0 - high_freq_fBm, saturate(height_fraction * 10.0));
        
        // 根据SIG 2017中的做法，使用Remap将扭曲的高频worley噪声用于侵蚀基础云的形状以塑造细节
        final_cloud = remap(cloud_with_coverage, saturate(high_freq_noise_modifier*0.5), 1.0, 0.0, 1.0);
    }
    
    return final_cloud;
}

const vec3 noise_kernel[6] = {
    vec3( 0.38051305,  0.92453449, -0.02111345),
    vec3(-0.50625799, -0.03590792, -0.86163418),
    vec3(-0.32509218, -0.94557439,  0.01428793),
    vec3( 0.09026238, -0.27376545,  0.95755165),
    vec3( 0.28128598,  0.42443639, -0.86065785),
    vec3(-0.16852403,  0.14748697,  0.97460106)
};

vec3 SampleCloudDensityAlongCone(vec3 p, float stepSize, int mip_level, float cos_angle, float density_along_view_cone)
{
    // 生成圆锥信息
    vec3 light_step = stepSize * uSunDirection;
    float cone_spread_multiplier = stepSize;

    float coneRadius = 1.0;
    float coneStep = 1.0 / 6;
    float density_along_cone = 0.0;
    float lod = 0.0;
    float lodStride = 1.0 / 6;
    vec3 weather_data = vec3(0.0);

    // 每次步进做6份锥形光照采样；
    // 如果沿着视线行进的累积密度已经超过其有效光贡献阈值【Horizon使用0.3】，则切换到低频采样模式以进一步优化
    for(int i=0; i<=6; i++)
    {
        // 将当前步长偏移添加到采样位置
        vec3 conePos = p + coneRadius * ( cone_spread_multiplier * noise_kernel[i] * float(i) );

        // 全采样模式，使用低level的mipmap
        int mip_offset = int(i * 0.5);

        weather_data = SampleWeather(conePos);

        if (density_along_view_cone < 0.3)
        {
            density_along_cone += SampleCloudDensity(conePos, weather_data, mip_level + mip_offset, false);
        }
        else
        {
            density_along_cone += SampleCloudDensity(conePos, weather_data, mip_level + mip_offset, true);
        }
        

        p += light_step;
        coneRadius += coneStep;
        lod += lodStride;
    }

    // 逐步进采样点计算光照
    return GetLightEnergy(p, density_along_cone, lod, cos_angle, GetHeightFractionForPoint(p, vec2(cloudBottomRadius, cloudTopRadius)));
}

vec4 rayMarchCloud(vec3 worldPos, vec3 worldDir)
{
    float viewHeight = length(worldPos);
    float tMin, tMax;
    if (viewHeight < cloudBottomRadius)
    {
        float tEarth = raySphereIntersectNearest(worldPos, worldDir, vec3(0.0), uGroundRadius);
        if (tEarth > 0.0)
            return vec4(0.0, 0.0, 0.0, 1.0);
        
        tMin = raySphereIntersectInside(worldPos, worldDir, vec3(0.0), cloudBottomRadius);
        tMax = raySphereIntersectInside(worldPos, worldDir, vec3(0.0), cloudTopRadius);
    }
    else if(viewHeight > cloudTopRadius)
    {
        vec2 t0t1 = vec2(0.0);
        const bool bIntersectionEnd = raySphereIntersectOutSide(worldPos, worldDir, vec3(0.0), cloudTopRadius, t0t1);
        if (!bIntersectionEnd)
        {
            return vec4(0.0, 0.0, 0.0, 1.0);
        }

        vec2 t2t3 = vec2(0.0);
        const bool bIntersectionStart = raySphereIntersectOutSide(worldPos, worldDir, vec3(0.0), cloudBottomRadius, t2t3);
        if (bIntersectionStart)
        {
            tMin = t0t1.x;
            tMax = t2t3.x;
        }
        else
        {
            tMin = t0t1.x;
            tMax = t0t1.y;
        }
    }
    else
    {
        float tStart = raySphereIntersectNearest(worldPos, worldDir, vec3(0.0), cloudBottomRadius);
        if (tStart > 0.0)
        {
            tMax = tStart;
        }
        else
        {
            tMax = raySphereIntersectInside(worldPos, worldDir, vec3(0.0), cloudTopRadius);
        }
        tMin = 0.0;
    }
    tMin = max(tMin, 0.0);
    tMax = max(tMax, 0.0);

    if (tMax <= tMin)
        return vec4(0.0, 0.0, 0.0, 1.0);

    const float stepSize = (tMax - tMin) / cloudMarchingStepCount;//0.0625; // 16 sample per 1 km
    float sampleT = tMin + stepSize * 0.001;
    float sampleDensityPrevious = -1.0;
    uint zeroDensitySampleCount = 0;
    float cloudTest = 0.0;
    float alpha = 0.0;
    const float cosTheta = dot(-worldDir, uSunDirection);

    vec3 cloudLighting = vec3(0.0);

    for (uint i = 0; i < cloudMarchingStepCount; ++i)
    {
        if (alpha <= 1.0)
        {
            vec3 samplePos = worldPos + sampleT * worldDir;
            vec3 weatherData = SampleWeather(samplePos);
            if (cloudTest > 0.0)
            {
                float sampleDensity = SampleCloudDensity(samplePos, weatherData, 0, false);

                if (sampleDensity == 0.0 && sampleDensityPrevious == 0.0)
                {
                    sampleDensityPrevious++;
                }

                if (zeroDensitySampleCount < 11 && sampleDensity != 0.0)
                {
                    // 昂贵的圆锥采样
                    alpha += sampleDensity;
                    cloudLighting += SampleCloudDensityAlongCone(samplePos, stepSize, 0, cosTheta, alpha);
                }
                else
                {
                    // 回到便宜的采样
                    cloudTest = 0.0;
                    zeroDensitySampleCount = 0;
                }
                sampleT += stepSize;
                sampleDensityPrevious = sampleDensity;
            }
            else
            {
                cloudTest = SampleCloudDensity(samplePos, weatherData, 0, true);
                if (cloudTest == 0.0)
                {
                    sampleT += stepSize * 2;
                }
                else
                {
                    sampleT -= stepSize;
                }
            }
        }
    }

    return vec4(cloudLighting, 1.0);
}

void main()
{
    vec4 clipSpace = vec4(uv * 2.0 - 1.0, 1.0, 1.0);
    vec4 viewSpace = uInverseProjectionMatrix * clipSpace;
    viewSpace *= 1.0 / viewSpace.w;
    vec3 worldDir = normalize(mat3(uInverseViewMatrix) * viewSpace.xyz);
    vec3 worldPos = getWorldPos(uInverseViewMatrix[3].xyz);
    float viewHeight = length(worldPos);
    fragData = vec4(rayMarchCloud(worldPos, worldDir));
}
