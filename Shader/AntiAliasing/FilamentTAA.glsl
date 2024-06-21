#version 410 core

#extension GL_ARB_shading_language_packing : enable

#extension GL_GOOGLE_cpp_style_line_directive : enable
#define SPIRV_CROSS_CONSTANT_ID_0 3
#define SPIRV_CROSS_CONSTANT_ID_1 64
#define SPIRV_CROSS_CONSTANT_ID_4 2048
#define SPIRV_CROSS_CONSTANT_ID_6 false
#define SPIRV_CROSS_CONSTANT_ID_7 false
#define SPIRV_CROSS_CONSTANT_ID_2 false
#define SPIRV_CROSS_CONSTANT_ID_5 false
#define SPIRV_CROSS_CONSTANT_ID_8 2


#define TARGET_GL_ENVIRONMENT
#define FILAMENT_OPENGL_SEMANTICS
#define FILAMENT_HAS_FEATURE_TEXTURE_GATHER
#define FILAMENT_HAS_FEATURE_INSTANCING
#define FILAMENT_EFFECTIVE_VERSION __VERSION__
#define VARYING in
#define SHADING_MODEL_UNLIT
#define FILAMENT_QUALITY_LOW    0
#define FILAMENT_QUALITY_NORMAL 1
#define FILAMENT_QUALITY_HIGH   2
#define FILAMENT_QUALITY FILAMENT_QUALITY_HIGH

precision highp float;
precision highp int;

#ifndef SPIRV_CROSS_CONSTANT_ID_0
#define SPIRV_CROSS_CONSTANT_ID_0 1
#endif
const int BACKEND_FEATURE_LEVEL = SPIRV_CROSS_CONSTANT_ID_0;

#ifndef SPIRV_CROSS_CONSTANT_ID_1
#define SPIRV_CROSS_CONSTANT_ID_1 64
#endif
const int CONFIG_MAX_INSTANCES = SPIRV_CROSS_CONSTANT_ID_1;

#ifndef SPIRV_CROSS_CONSTANT_ID_4
#define SPIRV_CROSS_CONSTANT_ID_4 1024
#endif
const int CONFIG_FROXEL_BUFFER_HEIGHT = SPIRV_CROSS_CONSTANT_ID_4;

#ifndef SPIRV_CROSS_CONSTANT_ID_6
#define SPIRV_CROSS_CONSTANT_ID_6 false
#endif
const bool CONFIG_DEBUG_DIRECTIONAL_SHADOWMAP = SPIRV_CROSS_CONSTANT_ID_6;

#ifndef SPIRV_CROSS_CONSTANT_ID_7
#define SPIRV_CROSS_CONSTANT_ID_7 false
#endif
const bool CONFIG_DEBUG_FROXEL_VISUALIZATION = SPIRV_CROSS_CONSTANT_ID_7;

#ifndef SPIRV_CROSS_CONSTANT_ID_2
#define SPIRV_CROSS_CONSTANT_ID_2 false
#endif
const bool CONFIG_STATIC_TEXTURE_TARGET_WORKAROUND = SPIRV_CROSS_CONSTANT_ID_2;

#ifndef SPIRV_CROSS_CONSTANT_ID_5
#define SPIRV_CROSS_CONSTANT_ID_5 false
#endif
const bool CONFIG_POWER_VR_SHADER_WORKAROUNDS = SPIRV_CROSS_CONSTANT_ID_5;

#ifndef SPIRV_CROSS_CONSTANT_ID_8
#define SPIRV_CROSS_CONSTANT_ID_8 2
#endif
const int CONFIG_STEREO_EYE_COUNT = SPIRV_CROSS_CONSTANT_ID_8;

#define CONFIG_MAX_STEREOSCOPIC_EYES 4

#if defined(GL_GOOGLE_cpp_style_line_directive)
#line 0                      
#endif
#if defined(FILAMENT_VULKAN_SEMANTICS)
#define LAYOUT_LOCATION(x) layout(location = x)
#else
#define LAYOUT_LOCATION(x)
#endif

#define bool2    bvec2
#define bool3    bvec3
#define bool4    bvec4

#define int2     ivec2
#define int3     ivec3
#define int4     ivec4

#define uint2    uvec2
#define uint3    uvec3
#define uint4    uvec4

#define float2   vec2
#define float3   vec3
#define float4   vec4

#define float3x3 mat3
#define float4x4 mat4

// To workaround an adreno crash (#5294), we need ensure that a method with
// parameter 'const mat4' does not call another method also with a 'const mat4'
// parameter (i.e. mulMat4x4Float3). So we remove the const modifier for
// materials compiled for vulkan+mobile.
#if defined(TARGET_VULKAN_ENVIRONMENT) && defined(TARGET_MOBILE)
   #define highp_mat4 highp mat4
#else
   #define highp_mat4 const highp mat4
#endif

#define POST_PROCESS_OPAQUE 1

LAYOUT_LOCATION(0) VARYING highp vec4 variable_vertex;

layout(std140) uniform FrameUniforms {
    mat4 viewFromWorldMatrix;
    mat4 worldFromViewMatrix;
    mat4 clipFromViewMatrix;
    mat4 viewFromClipMatrix;
    mat4 clipFromWorldMatrix[4];
    mat4 worldFromClipMatrix;
    mat4 userWorldFromWorldMatrix;
    vec4 clipTransform;
    vec2 clipControl;
    float time;
    float temporalNoise;
    vec4 userTime;
    vec4 resolution;
    vec2 logicalViewportScale;
    vec2 logicalViewportOffset;
    float lodBias;
    float refractionLodOffset;
    float oneOverFarMinusNear;
    float nearOverFarMinusNear;
    float cameraFar;
    float exposure;
    float ev100;
    float needsAlphaChannel;
    lowp float aoSamplingQualityAndEdgeDistance;
    lowp float aoBentNormals;
    lowp float aoReserved0;
    lowp float aoReserved1;
    lowp vec4 zParams;
    lowp uvec3 fParams;
    lowp int lightChannels;
    lowp vec2 froxelCountXY;
    float iblLuminance;
    float iblRoughnessOneLevel;
    lowp vec3 iblSH[9];
    vec3 lightDirection;
    lowp float padding0;
    vec4 lightColorIntensity;
    vec4 sun;
    vec2 shadowFarAttenuationParams;
    lowp int directionalShadows;
    lowp float ssContactShadowDistance;
    vec4 cascadeSplits;
    lowp int cascades;
    lowp float shadowPenumbraRatioScale;
    vec2 lightFarAttenuationParams;
    lowp float vsmExponent;
    lowp float vsmDepthScale;
    lowp float vsmLightBleedReduction;
    lowp uint shadowSamplingType;
    vec3 fogDensity;
    float fogStart;
    float fogMaxOpacity;
    uint fogMinMaxMip;
    float fogHeightFalloff;
    float fogCutOffDistance;
    vec3 fogColor;
    float fogColorFromIbl;
    float fogInscatteringStart;
    float fogInscatteringSize;
    float fogOneOverFarMinusNear;
    float fogNearOverFarMinusNear;
    mat3 fogFromWorldMatrix;
    mat4 ssrReprojection;
    mat4 ssrUvFromViewMatrix;
    lowp float ssrThickness;
    lowp float ssrBias;
    lowp float ssrDistance;
    lowp float ssrStride;
    vec4 custom[4];
    int rec709;
    lowp float es2Reserved0;
    lowp float es2Reserved1;
    lowp float es2Reserved2;
    lowp vec4 reserved[40];
} frameUniforms;

layout(std140) uniform MaterialParams {
    float alpha;
    mat4 reprojection;
    float filterWeights[9];
} materialParams;
uniform  sampler2D materialParams_color;
uniform highp sampler2D materialParams_depth;
uniform  sampler2D materialParams_history;

#if defined(GL_GOOGLE_cpp_style_line_directive)
#line 0                   
#endif
//------------------------------------------------------------------------------
// Common math
//------------------------------------------------------------------------------

/** @public-api */
#define PI                 3.14159265359
/** @public-api */
#define HALF_PI            1.570796327

#define MEDIUMP_FLT_MAX    65504.0
#define MEDIUMP_FLT_MIN    0.00006103515625

#ifdef TARGET_MOBILE
#define FLT_EPS            MEDIUMP_FLT_MIN
#define saturateMediump(x) min(x, MEDIUMP_FLT_MAX)
#else
#define FLT_EPS            1e-5
#define saturateMediump(x) x
#endif

#define saturate(x)        clamp(x, 0.0, 1.0)
#define atan2(x, y)        atan(y, x)

//------------------------------------------------------------------------------
// Scalar operations
//------------------------------------------------------------------------------

/**
 * Computes x^5 using only multiply operations.
 *
 * @public-api
 */
float pow5(float x) {
    float x2 = x * x;
    return x2 * x2 * x;
}

/**
 * Computes x^2 as a single multiplication.
 *
 * @public-api
 */
float sq(float x) {
    return x * x;
}

//------------------------------------------------------------------------------
// Vector operations
//------------------------------------------------------------------------------

/**
 * Returns the maximum component of the specified vector.
 *
 * @public-api
 */
float max3(const vec3 v) {
    return max(v.x, max(v.y, v.z));
}

float vmax(const vec2 v) {
    return max(v.x, v.y);
}

float vmax(const vec3 v) {
    return max(v.x, max(v.y, v.z));
}

float vmax(const vec4 v) {
    return max(max(v.x, v.y), max(v.y, v.z));
}

/**
 * Returns the minimum component of the specified vector.
 *
 * @public-api
 */
float min3(const vec3 v) {
    return min(v.x, min(v.y, v.z));
}

float vmin(const vec2 v) {
    return min(v.x, v.y);
}

float vmin(const vec3 v) {
    return min(v.x, min(v.y, v.z));
}

float vmin(const vec4 v) {
    return min(min(v.x, v.y), min(v.y, v.z));
}

//------------------------------------------------------------------------------
// Trigonometry
//------------------------------------------------------------------------------

/**
 * Approximates acos(x) with a max absolute error of 9.0x10^-3.
 * Valid in the range -1..1.
 */
float acosFast(float x) {
    // Lagarde 2014, "Inverse trigonometric functions GPU optimization for AMD GCN architecture"
    // This is the approximation of degree 1, with a max absolute error of 9.0x10^-3
    float y = abs(x);
    float p = -0.1565827 * y + 1.570796;
    p *= sqrt(1.0 - y);
    return x >= 0.0 ? p : PI - p;
}

/**
 * Approximates acos(x) with a max absolute error of 9.0x10^-3.
 * Valid only in the range 0..1.
 */
float acosFastPositive(float x) {
    float p = -0.1565827 * x + 1.570796;
    return p * sqrt(1.0 - x);
}

//------------------------------------------------------------------------------
// Matrix and quaternion operations
//------------------------------------------------------------------------------

/**
 * Multiplies the specified 3-component vector by the 4x4 matrix (m * v) in
 * high precision.
 *
 * @public-api
 */
highp vec4 mulMat4x4Float3(const highp mat4 m, const highp vec3 v) {
    return v.x * m[0] + (v.y * m[1] + (v.z * m[2] + m[3]));
}

/**
 * Multiplies the specified 3-component vector by the 3x3 matrix (m * v) in
 * high precision.
 *
 * @public-api
 */
highp vec3 mulMat3x3Float3(const highp mat4 m, const highp vec3 v) {
    return v.x * m[0].xyz + (v.y * m[1].xyz + (v.z * m[2].xyz));
}

/**
 * Extracts the normal vector of the tangent frame encoded in the specified quaternion.
 */
void toTangentFrame(const highp vec4 q, out highp vec3 n) {
    n = vec3( 0.0,  0.0,  1.0) +
        vec3( 2.0, -2.0, -2.0) * q.x * q.zwx +
        vec3( 2.0,  2.0, -2.0) * q.y * q.wzy;
}

/**
 * Extracts the normal and tangent vectors of the tangent frame encoded in the
 * specified quaternion.
 */
void toTangentFrame(const highp vec4 q, out highp vec3 n, out highp vec3 t) {
    toTangentFrame(q, n);
    t = vec3( 1.0,  0.0,  0.0) +
        vec3(-2.0,  2.0, -2.0) * q.y * q.yxw +
        vec3(-2.0,  2.0,  2.0) * q.z * q.zwx;
}

highp mat3 cofactor(const highp mat3 m) {
    highp float a = m[0][0];
    highp float b = m[1][0];
    highp float c = m[2][0];
    highp float d = m[0][1];
    highp float e = m[1][1];
    highp float f = m[2][1];
    highp float g = m[0][2];
    highp float h = m[1][2];
    highp float i = m[2][2];

    highp mat3 cof;
    cof[0][0] = e * i - f * h;
    cof[0][1] = c * h - b * i;
    cof[0][2] = b * f - c * e;
    cof[1][0] = f * g - d * i;
    cof[1][1] = a * i - c * g;
    cof[1][2] = c * d - a * f;
    cof[2][0] = d * h - e * g;
    cof[2][1] = b * g - a * h;
    cof[2][2] = a * e - b * d;
    return cof;
}

//------------------------------------------------------------------------------
// Random
//------------------------------------------------------------------------------

/*
 * Random number between 0 and 1, using interleaved gradient noise.
 * w must not be normalized (e.g. window coordinates)
 */
float interleavedGradientNoise(highp vec2 w) {
    const vec3 m = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(m.z * fract(dot(w, m.xy)));
}
#if defined(GL_GOOGLE_cpp_style_line_directive)
#line 0                    
#endif
// These variables should be in a struct but some GPU drivers ignore the
// precision qualifier on individual struct members
highp mat3  shading_tangentToWorld;   // TBN matrix
highp vec3  shading_position;         // position of the fragment in world space
      vec3  shading_view;             // normalized vector from the fragment to the eye
      vec3  shading_normal;           // normalized transformed normal, in world space
      vec3  shading_geometricNormal;  // normalized geometric normal, in world space
      vec3  shading_reflected;        // reflection of view about normal
      float shading_NoV;              // dot(normal, view), always strictly >= MIN_N_DOT_V

#if defined(MATERIAL_HAS_BENT_NORMAL)
      vec3  shading_bentNormal;       // normalized transformed normal, in world space
#endif

#if defined(MATERIAL_HAS_CLEAR_COAT)
      vec3  shading_clearCoatNormal;  // normalized clear coat layer normal, in world space
#endif

highp vec2 shading_normalizedViewportCoord;
#if defined(GL_GOOGLE_cpp_style_line_directive)
#line 0                     
#endif
//------------------------------------------------------------------------------
// Common color operations
//------------------------------------------------------------------------------

/**
 * Computes the luminance of the specified linear RGB color using the
 * luminance coefficients from Rec. 709.
 *
 * @public-api
 */
float luminance(const vec3 linear) {
    return dot(linear, vec3(0.2126, 0.7152, 0.0722));
}

/**
 * Computes the pre-exposed intensity using the specified intensity and exposure.
 * This function exists to force highp precision on the two parameters
 */
float computePreExposedIntensity(const highp float intensity, const highp float exposure) {
    return intensity * exposure;
}

void unpremultiply(inout vec4 color) {
    color.rgb /= max(color.a, FLT_EPS);
}

/**
 * Applies a full range YCbCr to sRGB conversion and returns an RGB color.
 *
 * @public-api
 */
vec3 ycbcrToRgb(float luminance, vec2 cbcr) {
    // Taken from https://developer.apple.com/documentation/arkit/arframe/2867984-capturedimage
    const mat4 ycbcrToRgbTransform = mat4(
         1.0000,  1.0000,  1.0000,  0.0000,
         0.0000, -0.3441,  1.7720,  0.0000,
         1.4020, -0.7141,  0.0000,  0.0000,
        -0.7010,  0.5291, -0.8860,  1.0000
    );
    return (ycbcrToRgbTransform * vec4(luminance, cbcr, 1.0)).rgb;
}

//------------------------------------------------------------------------------
// Tone mapping operations
//------------------------------------------------------------------------------

/*
 * The input must be in the [0, 1] range.
 */
vec3 Inverse_Tonemap_Filmic(const vec3 x) {
    return (0.03 - 0.59 * x - sqrt(0.0009 + 1.3702 * x - 1.0127 * x * x)) / (-5.02 + 4.86 * x);
}

/**
 * Applies the inverse of the tone mapping operator to the specified HDR or LDR
 * sRGB (non-linear) color and returns a linear sRGB color. The inverse tone mapping
 * operator may be an approximation of the real inverse operation.
 *
 * @public-api
 */
vec3 inverseTonemapSRGB(vec3 color) {
    // sRGB input
    color = clamp(color, 0.0, 1.0);
    return Inverse_Tonemap_Filmic(pow(color, vec3(2.2)));
}

/**
 * Applies the inverse of the tone mapping operator to the specified HDR or LDR
 * linear RGB color and returns a linear RGB color. The inverse tone mapping operator
 * may be an approximation of the real inverse operation.
 *
 * @public-api
 */
vec3 inverseTonemap(vec3 linear) {
    // Linear input
    return Inverse_Tonemap_Filmic(clamp(linear, 0.0, 1.0));
}

//------------------------------------------------------------------------------
// Common texture operations
//------------------------------------------------------------------------------

/**
 * Decodes the specified RGBM value to linear HDR RGB.
 */
vec3 decodeRGBM(vec4 c) {
    c.rgb *= (c.a * 16.0);
    return c.rgb * c.rgb;
}

//------------------------------------------------------------------------------
// Common screen-space operations
//------------------------------------------------------------------------------

// returns the frag coord in the GL convention with (0, 0) at the bottom-left
// resolution : width, height
highp vec2 getFragCoord(const highp vec2 resolution) {
#if defined(TARGET_METAL_ENVIRONMENT) || defined(TARGET_VULKAN_ENVIRONMENT)
    return vec2(gl_FragCoord.x, resolution.y - gl_FragCoord.y);
#else
    return gl_FragCoord.xy;
#endif
}

//------------------------------------------------------------------------------
// Common debug
//------------------------------------------------------------------------------

vec3 heatmap(float v) {
    vec3 r = v * 2.1 - vec3(1.8, 1.14, 0.3);
    return 1.0 - r * r;
}
#if defined(GL_GOOGLE_cpp_style_line_directive)
#line 0                      
#endif
//------------------------------------------------------------------------------
// Uniforms access
//------------------------------------------------------------------------------

/** @public-api */
highp mat4 getViewFromWorldMatrix() {
    return frameUniforms.viewFromWorldMatrix;
}

/** @public-api */
highp mat4 getWorldFromViewMatrix() {
    return frameUniforms.worldFromViewMatrix;
}

/** @public-api */
highp mat4 getClipFromViewMatrix() {
    return frameUniforms.clipFromViewMatrix;
}

/** @public-api */
highp mat4 getViewFromClipMatrix() {
    return frameUniforms.viewFromClipMatrix;
}

/** @public-api */
highp mat4 getClipFromWorldMatrix() {
#if defined(VARIANT_HAS_INSTANCED_STEREO)
    int eye = instance_index % CONFIG_STEREO_EYE_COUNT;
    return frameUniforms.clipFromWorldMatrix[eye];
#else
    return frameUniforms.clipFromWorldMatrix[0];
#endif
}

/** @public-api */
highp mat4 getWorldFromClipMatrix() {
    return frameUniforms.worldFromClipMatrix;
}

/** @public-api */
highp mat4 getUserWorldFromWorldMatrix() {
    return frameUniforms.userWorldFromWorldMatrix;
}

/** @public-api */
float getTime() {
    return frameUniforms.time;
}

/** @public-api */
highp vec4 getUserTime() {
    return frameUniforms.userTime;
}

/** @public-api **/
highp float getUserTimeMod(float m) {
    return mod(mod(frameUniforms.userTime.x, m) + mod(frameUniforms.userTime.y, m), m);
}

/**
 * Transforms a texture UV to make it suitable for a render target attachment.
 *
 * In Vulkan and Metal, texture coords are Y-down but in OpenGL they are Y-up. This wrapper function
 * accounts for these differences. When sampling from non-render targets (i.e. uploaded textures)
 * these differences do not matter because OpenGL has a second piece of backwardness, which is that
 * the first row of texels in glTexImage2D is interpreted as the bottom row.
 *
 * To protect users from these differences, we recommend that materials in the SURFACE domain
 * leverage this wrapper function when sampling from offscreen render targets.
 *
 * @public-api
 */
highp vec2 uvToRenderTargetUV(const highp vec2 uv) {
#if defined(TARGET_METAL_ENVIRONMENT) || defined(TARGET_VULKAN_ENVIRONMENT)
    return vec2(uv.x, 1.0 - uv.y);
#else
    return uv;
#endif
}

// TODO: below shouldn't be accessible from post-process materials

/** @public-api */
highp vec4 getResolution() {
    return frameUniforms.resolution;
}

/** @public-api */
highp vec3 getWorldCameraPosition() {
    return frameUniforms.worldFromViewMatrix[3].xyz;
}

/** @public-api, @deprecated use getUserWorldPosition() or getUserWorldFromWorldMatrix() instead  */
highp vec3 getWorldOffset() {
    return getUserWorldFromWorldMatrix()[3].xyz;
}

/** @public-api */
float getExposure() {
    // NOTE: this is a highp uniform only to work around #3602 (qualcomm)
    // We are intentionally casting it to mediump here, as per the Materials doc.
    return frameUniforms.exposure;
}

/** @public-api */
float getEV100() {
    return frameUniforms.ev100;
}

//------------------------------------------------------------------------------
// user defined globals
//------------------------------------------------------------------------------

highp vec4 getMaterialGlobal0() {
    return frameUniforms.custom[0];
}

highp vec4 getMaterialGlobal1() {
    return frameUniforms.custom[1];
}

highp vec4 getMaterialGlobal2() {
    return frameUniforms.custom[2];
}

highp vec4 getMaterialGlobal3() {
    return frameUniforms.custom[3];
}

#define FRAG_OUTPUT0 color
#define FRAG_OUTPUT_AT0 output_color
#define FRAG_OUTPUT_MATERIAL_TYPE0 vec4
#define FRAG_OUTPUT_PRECISION0 
#define FRAG_OUTPUT_TYPE0 vec4
#define FRAG_OUTPUT_SWIZZLE0 

layout(location=0) out  vec4 output_color;
#if defined(GL_GOOGLE_cpp_style_line_directive)
#line 0                         
#endif

struct PostProcessInputs {
#if defined(FRAG_OUTPUT0)
    FRAG_OUTPUT_PRECISION0 FRAG_OUTPUT_MATERIAL_TYPE0 FRAG_OUTPUT0;
#endif
#if defined(FRAG_OUTPUT1)
    FRAG_OUTPUT_PRECISION1 FRAG_OUTPUT_MATERIAL_TYPE1 FRAG_OUTPUT1;
#endif
#if defined(FRAG_OUTPUT2)
    FRAG_OUTPUT_PRECISION2 FRAG_OUTPUT_MATERIAL_TYPE2 FRAG_OUTPUT2;
#endif
#if defined(FRAG_OUTPUT3)
    FRAG_OUTPUT_PRECISION3 FRAG_OUTPUT_MATERIAL_TYPE3 FRAG_OUTPUT3;
#endif
#if defined(FRAG_OUTPUT4)
    FRAG_OUTPUT_PRECISION4 FRAG_OUTPUT_MATERIAL_TYPE4 FRAG_OUTPUT4;
#endif
#if defined(FRAG_OUTPUT5)
    FRAG_OUTPUT_PRECISION5 FRAG_OUTPUT_MATERIAL_TYPE5 FRAG_OUTPUT5;
#endif
#if defined(FRAG_OUTPUT6)
    FRAG_OUTPUT_PRECISION6 FRAG_OUTPUT_MATERIAL_TYPE6 FRAG_OUTPUT6;
#endif
#if defined(FRAG_OUTPUT7)
    FRAG_OUTPUT_PRECISION7 FRAG_OUTPUT_MATERIAL_TYPE7 FRAG_OUTPUT7;
#endif
#if defined(FRAG_OUTPUT_DEPTH)
    float depth;
#endif
};
#line 45
#if defined(GL_GOOGLE_cpp_style_line_directive)
#line 44          
#endif


#if defined(TARGET_METAL_ENVIRONMENT) || defined(TARGET_VULKAN_ENVIRONMENT)
#define TEXTURE_SPACE_UP    -1
#define TEXTURE_SPACE_DN     1
#else
#define TEXTURE_SPACE_UP     1
#define TEXTURE_SPACE_DN    -1
#endif

/* Clipping box type */

// min/max neighborhood
#define BOX_TYPE_AABB           0
// Variance based neighborhood
#define BOX_TYPE_VARIANCE       1
// uses both min/max and variance
#define BOX_TYPE_AABB_VARIANCE  2

// High VARIANCE_GAMMA [0.75, 1.25] increases ghosting artefact, lower values increases jittering
#define VARIANCE_GAMMA          1.0

/* Clipping algorithm */

// accurate box clipping
#define BOX_CLIPPING_ACCURATE   0
// clamping instead of clipping
#define BOX_CLIPPING_CLAMP      1
// no clipping (for debugging only)
#define BOX_CLIPPING_NONE       2

/* Configuration mobile/desktop */

#if FILAMENT_QUALITY < FILAMENT_QUALITY_HIGH
#define BOX_CLIPPING            BOX_CLIPPING_ACCURATE
#define BOX_TYPE                BOX_TYPE_VARIANCE
#define USE_YCoCg               0
#define FILTER_INPUT            1
#define FILTER_HISTORY          0
#else
#define BOX_CLIPPING            BOX_CLIPPING_ACCURATE
#define BOX_TYPE                BOX_TYPE_AABB_VARIANCE
#define USE_YCoCg               1
#define FILTER_INPUT            1
#define FILTER_HISTORY          1
#endif

/* debugging helper */

#define HISTORY_REPROJECTION    1
#define PREVENT_FLICKERING      0   // FIXME: thin lines disapear


float luma(const vec3 color) {
#if USE_YCoCg
    return color.x;
#else
    return luminance(color);
#endif
}

vec3 RGB_YCoCg(const vec3 c) {
    float Y  = dot(c.rgb, vec3( 1, 2,  1) * 0.25);
    float Co = dot(c.rgb, vec3( 2, 0, -2) * 0.25);
    float Cg = dot(c.rgb, vec3(-1, 2, -1) * 0.25);
    return vec3(Y, Co, Cg);
}

vec3 YCoCg_RGB(const vec3 c) {
    float Y  = c.x;
    float Co = c.y;
    float Cg = c.z;
    float r = Y + Co - Cg;
    float g = Y + Cg;
    float b = Y - Co - Cg;
    return vec3(r, g, b);
}

// clip the (c, h) segment to a box
vec4 clipToBox(const int quality,
        const vec3 boxmin,  const vec3 boxmax, const vec4 c, const vec4 h) {
    const float epsilon = 0.0001;

    if (quality == BOX_CLIPPING_ACCURATE) {
        vec4 r = c - h;
        vec3 ir = 1.0 / (epsilon + r.rgb);
        vec3 rmax = (boxmax - h.rgb) * ir;
        vec3 rmin = (boxmin - h.rgb) * ir;
        vec3 imin = min(rmax, rmin);
        return h + r * saturate(max3(imin));
    } else if (quality == BOX_CLIPPING_CLAMP) {
        return vec4(clamp(h.rgb, boxmin, boxmax), h.a);
    }
    return h;
}

// Samples a texture with Catmull-Rom filtering, using 9 texture fetches instead of 16.
//      https://therealmjp.github.io/
// Some optimizations from here:
//      http://vec3.ca/bicubic-filtering-in-fewer-taps/ for more details
// Optimized to 5 taps by removing the corner samples
// And modified for mediump support
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

void postProcess(inout PostProcessInputs postProcess) {
    highp vec4 uv = variable_vertex.xyxy; // interpolated to pixel center

    // read the depth buffer center sample for reprojection
    float depth = textureLod(materialParams_depth, uv.xy, 0.0).r;

#if HISTORY_REPROJECTION
    uv.zw = uvToRenderTargetUV(uv.zw);
    // reproject history to current frame
    highp vec4 q = materialParams.reprojection * vec4(uv.zw, depth, 1.0);
    uv.zw = (q.xy * (1.0 / q.w)) * 0.5 + 0.5;
    uv.zw = uvToRenderTargetUV(uv.zw);
#endif

    // read center color and history samples
    vec4 color = textureLod(materialParams_color, uv.xy, 0.0);
#if FILTER_HISTORY
    vec4 history = sampleTextureCatmullRom(materialParams_history, uv.zw,
            vec2(textureSize(materialParams_history, 0)));
#else
    vec4 history = textureLod(materialParams_history, uv.zw, 0.0);
#endif

#if USE_YCoCg
    history.rgb = RGB_YCoCg(history.rgb);
#endif

    // build the history clamping box
    vec3 s[9];
    s[0] = textureLodOffset(materialParams_color, uv.xy, 0.0, ivec2(-1, TEXTURE_SPACE_DN)).rgb;
    s[1] = textureLodOffset(materialParams_color, uv.xy, 0.0, ivec2( 0, TEXTURE_SPACE_DN)).rgb;
    s[2] = textureLodOffset(materialParams_color, uv.xy, 0.0, ivec2( 1, TEXTURE_SPACE_DN)).rgb;
    s[3] = textureLodOffset(materialParams_color, uv.xy, 0.0, ivec2(-1, 0)).rgb;
    s[4] = color.rgb;
    s[5] = textureLodOffset(materialParams_color, uv.xy, 0.0, ivec2( 1, 0)).rgb;
    s[6] = textureLodOffset(materialParams_color, uv.xy, 0.0, ivec2(-1, TEXTURE_SPACE_UP)).rgb;
    s[7] = textureLodOffset(materialParams_color, uv.xy, 0.0, ivec2( 0, TEXTURE_SPACE_UP)).rgb;
    s[8] = textureLodOffset(materialParams_color, uv.xy, 0.0, ivec2( 1, TEXTURE_SPACE_UP)).rgb;

#if USE_YCoCg
    for (int i = 0 ; i < 9 ; i++) {
        s[i] = RGB_YCoCg(s[i]);
    }
    color.rgb = s[4].rgb;
#endif

#if FILTER_INPUT
    // unjitter/filter input
    vec4 filtered = vec4(0, 0, 0, color.a);
    for (int i = 0 ; i < 9 ; i++) {
        filtered.rgb += s[i] * materialParams.filterWeights[i];
    }
#else
    vec4 filtered = color;
#endif

#if BOX_TYPE == BOX_TYPE_AABB || BOX_TYPE == BOX_TYPE_AABB_VARIANCE
    vec3 boxmin = min(s[4], min(min(s[1], s[3]), min(s[5], s[7])));
    vec3 boxmax = max(s[4], max(max(s[1], s[3]), max(s[5], s[7])));
    vec3 box9min = min(boxmin, min(min(s[0], s[2]), min(s[6], s[8])));
    vec3 box9max = max(boxmax, max(max(s[0], s[2]), max(s[6], s[8])));
    // round the corners of the 3x3 box
    boxmin = (boxmin + box9min) * 0.5;
    boxmax = (boxmax + box9max) * 0.5;
#endif

#if BOX_TYPE == BOX_TYPE_VARIANCE || BOX_TYPE == BOX_TYPE_AABB_VARIANCE
    // "An Excursion in Temporal Supersampling" by Marco Salvi
    vec3 m0 = s[4];
    vec3 m1 = s[4] * s[4];
    // we use only 5 samples instead of all 9
    for (int i = 1 ; i < 9 ; i+=2) {
        m0 += s[i];
        m1 += s[i] * s[i];
    }
    vec3 a0 = m0 * (1.0 / 5.0);
    vec3 a1 = m1 * (1.0 / 5.0);
    vec3 sigma = sqrt(a1 - a0 * a0);
#if BOX_TYPE == BOX_TYPE_VARIANCE
    vec3 boxmin = a0 - VARIANCE_GAMMA * sigma;
    vec3 boxmax = a0 + VARIANCE_GAMMA * sigma;
#else
    boxmin = min(boxmin, a0 - VARIANCE_GAMMA * sigma);
    boxmax = max(boxmax, a0 + VARIANCE_GAMMA * sigma);
#endif
#endif

    // history clamping
    history = clipToBox(BOX_CLIPPING, boxmin, boxmax, filtered, history);

    float lumaColor   = luma(filtered.rgb);
    float lumaHistory = luma(history.rgb);

    float alpha = materialParams.alpha;
#if PREVENT_FLICKERING
    // [Lottes] prevents flickering by modulating the blend weight by the difference in luma
    float diff = 1.0 - abs(lumaColor - lumaHistory) / (0.001 + max(lumaColor, lumaHistory));
    alpha *= diff * diff;
#endif

    // tonemapping for handling HDR
    filtered.rgb *= 1.0 / (1.0 + lumaColor);
    history.rgb  *= 1.0 / (1.0 + lumaHistory);

    // combine history and current frame
    vec4 result = mix(history, filtered, alpha);

    // untonemap result
    result.rgb *= 1.0 / (1.0 - luma(result.rgb));

#if USE_YCoCg
    result.rgb = YCoCg_RGB(result.rgb);
#endif

    // store result (which will becomes new history)
    // we could end-up with negative values due to the bicubic filter, which never recover on
    // their own.
    result = max(vec4(0), result);

#if POST_PROCESS_OPAQUE
    // kill the work performed above
    result.a = 1.0;
#endif

    postProcess.color = result;
}

#line 989
#if defined(GL_GOOGLE_cpp_style_line_directive)
#line 0                  
#endif
void main() {
    PostProcessInputs inputs;

    // Invoke user code
    postProcess(inputs);

#if defined(FRAG_OUTPUT0)
    FRAG_OUTPUT_AT0 FRAG_OUTPUT_SWIZZLE0 = inputs.FRAG_OUTPUT0;
#endif
#if defined(FRAG_OUTPUT1)
    FRAG_OUTPUT_AT1 FRAG_OUTPUT_SWIZZLE1 = inputs.FRAG_OUTPUT1;
#endif
#if defined(FRAG_OUTPUT2)
    FRAG_OUTPUT_AT2 FRAG_OUTPUT_SWIZZLE2 = inputs.FRAG_OUTPUT2;
#endif
#if defined(FRAG_OUTPUT3)
    FRAG_OUTPUT_AT3 FRAG_OUTPUT_SWIZZLE3 = inputs.FRAG_OUTPUT3;
#endif
#if defined(FRAG_OUTPUT4)
    FRAG_OUTPUT_AT4 FRAG_OUTPUT_SWIZZLE4 = inputs.FRAG_OUTPUT4;
#endif
#if defined(FRAG_OUTPUT5)
    FRAG_OUTPUT_AT5 FRAG_OUTPUT_SWIZZLE5 = inputs.FRAG_OUTPUT5;
#endif
#if defined(FRAG_OUTPUT6)
    FRAG_OUTPUT_AT6 FRAG_OUTPUT_SWIZZLE6 = inputs.FRAG_OUTPUT6;
#endif
#if defined(FRAG_OUTPUT7)
    FRAG_OUTPUT_AT7 FRAG_OUTPUT_SWIZZLE7 = inputs.FRAG_OUTPUT7;
#endif
#if defined(FRAG_OUTPUT_DEPTH)
    gl_FragDepth = inputs.depth;
#endif
}

