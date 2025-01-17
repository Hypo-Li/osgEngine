#version 430 core
in vec2 uv;
out vec4 fragData;
uniform sampler2D uCurrentColorTexture;
uniform sampler2D uCurrentDepthTexture;
uniform sampler2D uHistoryColorTexture;
uniform sampler2D uMotionVectorTexture;
uniform vec4 uResolution;
uniform uint osg_FrameNumber;

layout(std140, binding = 0) uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uInverseViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uInverseProjectionMatrix;
    mat4 uReprojectionMatrix;
    vec2 uJitterPixels;
    vec2 uPrevJitterPixels;
};

float saturate(float x) { return clamp(x, 0.0, 1.0); }
vec2 saturate(vec2 x) { return clamp(x, vec2(0.0), vec2(1.0)); }
vec3 saturate(vec3 x) { return clamp(x, vec3(0.0), vec3(1.0)); }
vec4 saturate(vec4 x) { return clamp(x, vec4(0.0), vec4(1.0)); }

float rcp(float x)
{
    return 1.0 / x;
}

float max3(const vec3 v)
{
    return max(v.x, max(v.y, v.z));
}

float lumaYCoCg(const vec3 c)
{
    return c.x;
}

vec3 RGB_YCoCg(const vec3 c)
{
    float Y  = dot(c.rgb, vec3( 1, 2,  1));
    float Co = dot(c.rgb, vec3( 2, 0, -2));
    float Cg = dot(c.rgb, vec3(-1, 2, -1));
    return vec3(Y, Co, Cg);
}

vec3 YCoCg_RGB(const vec3 c)
{
    float Y  = c.x * 0.25;
    float Co = c.y * 0.25;
    float Cg = c.z * 0.25;
    float r = Y + Co - Cg;
    float g = Y + Cg;
    float b = Y - Co - Cg;
    return vec3(r, g, b);
}

uvec3 Rand3DPCG16(ivec3 p)
{
	uvec3 v = uvec3(p);
	v = v * 1664525u + 1013904223u;
	v.x += v.y*v.z;
	v.y += v.z*v.x;
	v.z += v.x*v.y;
	v.x += v.y*v.z;
	v.y += v.z*v.x;
	v.z += v.x*v.y;
	return v >> 16u;
}

vec2 Hammersley16( uint Index, uint NumSamples, uvec2 Random )
{
	float E1 = fract(float(Index / NumSamples) + float( Random.x ) * (1.0 / 65536.0));
	float E2 = float( ( bitfieldReverse(Index) >> 16 ) ^ Random.y ) * (1.0 / 65536.0);
	return vec2( E1, E2 );
}

vec3 QuantizeForFloatRenderTarget(vec3 Color, float E, vec3 QuantizationError)
{
	vec3 Error = Color * QuantizationError;

	Error.x = uintBitsToFloat(floatBitsToUint(Error.x) & ~0x007FFFFF);
	Error.y = uintBitsToFloat(floatBitsToUint(Error.y) & ~0x007FFFFF);
	Error.z = uintBitsToFloat(floatBitsToUint(Error.z) & ~0x007FFFFF);
	
	return Color + Error * E;
}

const ivec2 kOffsets3x3[9] =
{
	ivec2(-1, -1),
	ivec2( 0, -1),
	ivec2( 1, -1),
	ivec2(-1,  0),
	ivec2( 0,  0), 
	ivec2( 1,  0),
	ivec2(-1,  1),
	ivec2( 0,  1),
	ivec2( 1,  1)
};

const vec4 plusWeights[2] = {
    vec4(0.0344382524, 0.0303348377, 0.6590525508, 0.1468350291),
    vec4(0.1293392330, 0.0000000000, 0.0000000000, 1.0000000000)
};

const uint kPlusIndexes3x3[5] = { 1, 3, 4, 5, 7 };

const uint kSquareIndexes3x3[9] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };

void computeNeighborhoodBoundingbox(out vec4 outNeighborMin, out vec4 outNeighborMax)
{
    vec4 neighbors[9];

    neighbors[1].rgb = RGB_YCoCg(textureLodOffset(uCurrentColorTexture, uv, 0, kOffsets3x3[1]).rgb);
    neighbors[1].a = lumaYCoCg(neighbors[1].rgb);
    neighbors[3].rgb = RGB_YCoCg(textureLodOffset(uCurrentColorTexture, uv, 0, kOffsets3x3[3]).rgb);
    neighbors[3].a = lumaYCoCg(neighbors[3].rgb);
    neighbors[4].rgb = RGB_YCoCg(textureLodOffset(uCurrentColorTexture, uv, 0, kOffsets3x3[4]).rgb);
    neighbors[4].a = lumaYCoCg(neighbors[4].rgb);
    neighbors[5].rgb = RGB_YCoCg(textureLodOffset(uCurrentColorTexture, uv, 0, kOffsets3x3[5]).rgb);
    neighbors[5].a = lumaYCoCg(neighbors[5].rgb);
    neighbors[7].rgb = RGB_YCoCg(textureLodOffset(uCurrentColorTexture, uv, 0, kOffsets3x3[7]).rgb);
    neighbors[7].a = lumaYCoCg(neighbors[7].rgb);

    vec4 neighborMin, neighborMax;
    {
        neighborMin = min(min(neighbors[1], neighbors[3]), neighbors[4]);
        neighborMin = min(min(neighborMin, neighbors[5]), neighbors[7]);

        neighborMax = max(max(neighbors[1], neighbors[3]), neighbors[4]);
        neighborMax = max(max(neighborMax, neighbors[5]), neighbors[7]);
    }

    outNeighborMin = neighborMin;
    outNeighborMax = neighborMax;
}

vec4 sampleTextureCatmullRom(const sampler2D tex, const vec2 uv, const vec2 texSize) {
    // We're going to sample a a 4x4 grid of texels surrounding the target UV coordinate.
    // We'll do this by rounding down the sample location to get the exact center of our "starting"
    // texel. The starting texel will be at location [1, 1] in the grid, where [0, 0] is the
    // top left corner.

    highp vec2 samplePos = uv * texSize;
    highp vec2 texPos1 = floor(samplePos - 0.5) + 0.5;

    // Compute the fractional offset from our starting texel to our original sample location,
    // which we'll feed into the Catmull-Rom spline function to get our filter weights.
    highp vec2 f = samplePos - texPos1;
    highp vec2 f2 = f * f;
    highp vec2 f3 = f2 * f;

    // Compute the Catmull-Rom weights using the fractional offset that we calculated earlier.
    // These equations are pre-expanded based on our knowledge of where the texels will be located,
    // which lets us avoid having to evaluate a piece-wise function.
    vec2 w0 = f2 - 0.5 * (f3 + f);
    vec2 w1 = 1.5 * f3 - 2.5 * f2 + 1.0;
    vec2 w3 = 0.5 * (f3 - f2);
    vec2 w2 = 1.0 - w0 - w1 - w3;

    // Work out weighting factors and sampling offsets that will let us use bilinear filtering to
    // simultaneously evaluate the middle 2 samples from the 4x4 grid.
    vec2 w12 = w1 + w2;

    // Compute the final UV coordinates we'll use for sampling the texture
    highp vec2 texPos0 = texPos1 - vec2(1.0);
    highp vec2 texPos3 = texPos1 + vec2(2.0);
    highp vec2 texPos12 = texPos1 + w2 / w12;

    highp vec2 invTexSize = 1.0 / texSize;
    texPos0  *= invTexSize;
    texPos3  *= invTexSize;
    texPos12 *= invTexSize;

    float k0 = w12.x * w0.y;
    float k1 = w0.x  * w12.y;
    float k2 = w12.x * w12.y;
    float k3 = w3.x  * w12.y;
    float k4 = w12.x * w3.y;

    vec4 s[5];
    s[0] = textureLod(tex, vec2(texPos12.x, texPos0.y),  0.0);
    s[1] = textureLod(tex, vec2(texPos0.x,  texPos12.y), 0.0);
    s[2] = textureLod(tex, vec2(texPos12.x, texPos12.y), 0.0);
    s[3] = textureLod(tex, vec2(texPos3.x,  texPos12.y), 0.0);
    s[4] = textureLod(tex, vec2(texPos12.x, texPos3.y),  0.0);

    vec4 result =   k0 * s[0]
                  + k1 * s[1]
                  + k2 * s[2]
                  + k3 * s[3]
                  + k4 * s[4];

    result *= rcp(k0 + k1 + k2 + k3 + k4);

    // we could end-up with negative values
    result = max(vec4(0), result);

    return result;
}

vec4 sampleHistory(vec2 historyUV)
{
    vec4 history = sampleTextureCatmullRom(uHistoryColorTexture, historyUV, vec2(textureSize(uHistoryColorTexture, 0)));
    history.rgb = RGB_YCoCg(history.rgb);
    history.a = lumaYCoCg(history.rgb);
    return history;
}

vec4 clampHistory(vec4 filtered, vec4 history, vec4 neighborMin, vec4 neighborMax)
{
#if 0
    const float epsilon = 0.0001;
    vec3 rayOrigin = history.rgb;
    vec3 boxMin = neighborMin.rgb;
    vec3 boxMax = neighborMax.rgb;
    vec3 rayDir = filtered.rgb - history.rgb;
    vec3 invRayDir = 1.0  / (epsilon + rayDir);
    vec3 minIntersect = (boxMin - rayOrigin) * invRayDir;
    vec3 maxIntersect = (boxMax - rayOrigin) * invRayDir;
    vec3 enterIntersect = min(minIntersect, maxIntersect);
    history.rgb = history.rgb + rayDir * saturate(max3(enterIntersect));
    history.a = lumaYCoCg(history.rgb);
#else
    history = clamp(history, neighborMin, neighborMax);
#endif
    return history;
}

vec4 filterCurrentFrameInputSamples()
{
    float neighborsHdrWeight = 0;
    float neighborsFinalWeight = 0;
    vec3 neighborsColor = vec3(0);
    vec2 texelSize = uResolution.zw;

    for (uint i = 0; i < 5; ++i)
    {
        vec2 fSampleOffset = vec2(kOffsets3x3[kPlusIndexes3x3[i]]);
        float sampleSpatialWeight = plusWeights[i >> 2u][i & 3u];
        vec3 sampleColor = RGB_YCoCg(textureLod(uCurrentColorTexture, uv + fSampleOffset * texelSize, 0).rgb);
        float sampleHdrWeight = rcp(lumaYCoCg(sampleColor) + 4);
        float bilateralWeight = 1;
        float sampleFinalWeight = sampleSpatialWeight * sampleHdrWeight * bilateralWeight;
        neighborsColor += sampleFinalWeight * sampleColor;
        neighborsFinalWeight += sampleFinalWeight;
        neighborsHdrWeight += sampleSpatialWeight * sampleHdrWeight;
    }

    vec4 filtered;
    filtered.rgb = neighborsColor * rcp(neighborsFinalWeight);
    filtered.a = 1.0;
    return filtered;
}

void main()
{
    float alpha = texelFetch(uCurrentColorTexture, ivec2(gl_FragCoord.xy), 0).a;
    float depth = texelFetch(uCurrentDepthTexture, ivec2(gl_FragCoord.xy), 0).r;
    vec2 motionVector = texelFetch(uMotionVectorTexture, ivec2(gl_FragCoord.xy), 0).rg;
    
    vec4 q = uReprojectionMatrix * vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec2 reprojectedUV = (q.xy / q.w) * 0.5 + 0.5;
    reprojectedUV -= motionVector;
    vec2 backN = uv - reprojectedUV;
    if (motionVector.x > 0)
    {
        backN = motionVector.xy;
    }
    vec2 backTemp = backN * uResolution.xy;
    float velocity = length(backTemp);
    bool offScreen = max(abs(reprojectedUV.x), abs(reprojectedUV.y)) >= 1.0;

    // filter current frame
    vec4 filtered = filterCurrentFrameInputSamples();

    // compute neighborhood bounding box
    vec4 neighborMin, neighborMax;
    computeNeighborhoodBoundingbox(neighborMin, neighborMax);

    // sample history
    vec4 history = sampleHistory(reprojectedUV);
    float lumaHistory = history.a;

    history = clampHistory(filtered, history, neighborMin, neighborMax);

    float lumaFiltered = lumaYCoCg(filtered.rgb);
    float blendFactor = 0.04;
    {
        blendFactor = mix(blendFactor, 0.2, saturate(velocity / 40));
        blendFactor = max(blendFactor, saturate(0.01 * lumaHistory / abs(lumaFiltered - lumaHistory)));
        blendFactor = alpha != 1.0 ? mix(0.04, 0.25, 1.0 - alpha) : blendFactor;
        if (offScreen)
            blendFactor = 1.0;
    }

    float filterWeight = rcp(lumaFiltered + 4);
    float historyWeight = rcp(lumaYCoCg(history.rgb) + 4);

    vec2 weights;
    {
        float blendA = (1.0 - blendFactor) * historyWeight;
        float blendB = blendFactor * filterWeight;
        float rcpBlend = rcp(blendA + blendB);
        blendA *= rcpBlend;
        blendB *= rcpBlend;
        weights = vec2(blendA, blendB);
    }

    vec4 outputColor = history * weights.x + filtered * weights.y;
    outputColor.rgb = YCoCg_RGB(outputColor.rgb);

    uvec2 random = Rand3DPCG16(ivec3(gl_FragCoord.xy, osg_FrameNumber % 8)).xy;
    vec2 E = Hammersley16(0, 1, random);
    outputColor.rgb = QuantizeForFloatRenderTarget(outputColor.rgb, E.x, vec3(0.0009765625, 0.0009765625, 0.0009765625));

    fragData = vec4(outputColor.rgb, 1.0);
}