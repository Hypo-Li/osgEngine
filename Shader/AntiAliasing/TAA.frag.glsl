#version 430 core
in vec2 uv;
out vec4 fragData;
uniform sampler2D uCurrentColorTexture;
uniform sampler2D uCurrentDepthTexture;
uniform sampler2D uHistoryColorTexture;
uniform sampler2D uHistoryDepthTexture; // pre frame depth texture
uniform sampler2D uVelocityTexture;
uniform uint osg_FrameNumber;

// High VARIANCE_GAMMA [0.75, 1.25] increases ghosting artefact, lower values increases jittering
#define VARIANCE_GAMMA          1.0

layout(std140, binding = 0) uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uProjectionMatrixJittered;
    mat4 uInverseViewMatrix;
    mat4 uInverseProjectionMatrix;
    mat4 uInverseProjectionMatrixJittered;
    mat4 uReprojectionMatrix;
};

vec3 RGB2YCoCg(vec3 rgb)
{
    return mat3(
        0.25, 0.5, -0.25,
        0.5, 0.0, 0.5,
        0.25, -0.5, -0.25
    ) * rgb;
}

vec3 YCoCg2RGB(vec3 yCoCg)
{
    return mat3(
        1.0, 1.0, 1.0,
        1.0, 0.0, -1.0,
        -1.0, 1.0, -1.0
    ) * yCoCg;
}

float max3(const vec3 v) {
    return max(v.x, max(v.y, v.z));
}

vec4 clipToBox(const vec3 boxmin,  const vec3 boxmax, const vec4 c, const vec4 h)
{
    const float epsilon = 0.0001;
    vec4 r = c - h;
    vec3 ir = 1.0 / (epsilon + r.rgb);
    vec3 rmax = (boxmax - h.rgb) * ir;
    vec3 rmin = (boxmin - h.rgb) * ir;
    vec3 imin = min(rmax, rmin);
    return h + r * clamp(max3(imin), 0.0, 1.0);

    //return vec4(clamp(h.rgb, boxmin, boxmax), h.a);
    
    //return h;
}

const float filterWeights[9] = {
    0.0021563061, 0.0415091589, 0.0082372064,
    0.0338960849, 0.6525043845, 0.1294849515,
    0.0054927655, 0.1057364866, 0.0209826715
};

vec4 sampleTextureCatmullRom(const sampler2D tex, const highp vec2 uv, const highp vec2 texSize) {
    // We're going to sample a a 4x4 grid of texels surrounding the target UV coordinate. We'll do this by rounding
    // down the sample location to get the exact center of our "starting" texel. The starting texel will be at
    // location [1, 1] in the grid, where [0, 0] is the top left corner.

    highp vec2 samplePos = uv * texSize;
    highp vec2 texPos1 = floor(samplePos - 0.5) + 0.5;

    // Compute the fractional offset from our starting texel to our original sample location, which we'll
    // feed into the Catmull-Rom spline function to get our filter weights.
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

    vec4 result =   textureLod(tex, vec2(texPos12.x, texPos0.y),  0.0) * k0
                  + textureLod(tex, vec2(texPos0.x,  texPos12.y), 0.0) * k1
                  + textureLod(tex, vec2(texPos12.x, texPos12.y), 0.0) * k2
                  + textureLod(tex, vec2(texPos3.x,  texPos12.y), 0.0) * k3
                  + textureLod(tex, vec2(texPos12.x, texPos3.y),  0.0) * k4;

    result *= 1.0 / (k0 + k1 + k2 + k3 + k4);

    return result;
}

void main()
{
    float depth = textureLod(uCurrentDepthTexture, uv, 0.0).r;
    vec4 q = uReprojectionMatrix * vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec2 reprojectedUV = (q.xy * (1.0 / q.w)) * 0.5 + 0.5;
    
    vec4 color = textureLod(uCurrentColorTexture, uv, 0.0);
    //vec4 history = textureLod(uHistoryColorTexture, reprojectedUV, 0.0);
    vec4 history = sampleTextureCatmullRom(uHistoryColorTexture, reprojectedUV, vec2(textureSize(uHistoryColorTexture, 0)));
    
    history.rgb = RGB2YCoCg(history.rgb);
    vec3 s[9];
    s[0] = textureLodOffset(uCurrentColorTexture, uv, 0.0, ivec2(-1, -1)).rgb;
    s[1] = textureLodOffset(uCurrentColorTexture, uv, 0.0, ivec2( 0, -1)).rgb;
    s[2] = textureLodOffset(uCurrentColorTexture, uv, 0.0, ivec2( 1, -1)).rgb;
    s[3] = textureLodOffset(uCurrentColorTexture, uv, 0.0, ivec2(-1,  0)).rgb;
    s[4] = color.rgb;
    s[5] = textureLodOffset(uCurrentColorTexture, uv, 0.0, ivec2( 1,  0)).rgb;
    s[6] = textureLodOffset(uCurrentColorTexture, uv, 0.0, ivec2(-1,  1)).rgb;
    s[7] = textureLodOffset(uCurrentColorTexture, uv, 0.0, ivec2( 0,  1)).rgb;
    s[8] = textureLodOffset(uCurrentColorTexture, uv, 0.0, ivec2( 1,  1)).rgb;

    for (uint i = 0; i < 9; i++)
    {
        s[i] = RGB2YCoCg(s[i]);
    }
    color.rgb = s[4].rgb;
    
    vec4 filtered = vec4(0, 0, 0, color.a);
    for (int i = 0; i < 9; i++)
    {
        filtered.rgb += s[i] * filterWeights[i];
    }

    vec3 boxMin = min(s[4], min(min(s[1], s[3]), min(s[5], s[7])));
    vec3 boxMax = max(s[4], max(max(s[1], s[3]), max(s[5], s[7])));
    vec3 box9Min = min(boxMin, min(min(s[0], s[2]), min(s[6], s[8])));
    vec3 box9Max = max(boxMin, max(max(s[0], s[2]), max(s[6], s[8])));

    boxMin = (boxMin + box9Min) * 0.5;
    boxMax = (boxMax + box9Max) * 0.5;

    vec3 m0 = s[4];
    vec3 m1 = s[4] * s[4];
    for (int i = 1; i < 9; i += 2)
    {
        m0 += s[i];
        m1 += s[i] * s[i];
    }
    vec3 a0 = m0 * (1.0 / 5.0);
    vec3 a1 = m1 * (1.0 / 5.0);
    vec3 sigma = sqrt(a1 - a0 * a0);
    boxMin = min(boxMin, a0 - VARIANCE_GAMMA * sigma);
    boxMax = max(boxMax, a0 + VARIANCE_GAMMA * sigma);

    history = clipToBox(boxMin, boxMax, filtered, history);
    // YCoCg's luminance is x
    float lumaColor   = filtered.r;
    float lumaHistory = history.r;
    filtered.rgb *= 1.0 / (1.0 + lumaColor);
    history.rgb  *= 1.0 / (1.0 + lumaHistory);

    vec4 result = mix(history, filtered, vec4(0.05));
    result.rgb *= 1.0 / (1.0 - result.r);

    result.rgb = YCoCg2RGB(result.rgb);

    result = max(vec4(0), result);
    fragData = vec4(result.rgb, 1.0);
}