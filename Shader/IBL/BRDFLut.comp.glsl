#version 430 core
layout(local_size_x = 32, local_size_y = 32) in;
layout(binding = 0, rg16f) uniform image2D uBRDFLut;
const float PI = 3.14159265358979323846;
#define SAMPLE_COUNT 1024

vec2 hammersley(uint i, uint n)
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

vec3 importanceSamplingGGX(vec2 u, float a)
{
    float phi = 2.0 * PI * u.x;
    float cosTheta2 = (1 - u.y) / (1 + (a + 1) * ((a - 1) * u.y));
    float cosTheta = sqrt(cosTheta2);
    float sinTheta = sqrt(1.0 - cosTheta2);
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

// Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
float V_SmithGGXCorrelated(float ndv, float ndl, float a2)
{
    float ggxv = ndl * sqrt((ndv - a2 * ndv) * ndv + a2);
    float ggxl = ndv * sqrt((ndl - a2 * ndl) * ndl + a2);
    return 0.5 / (ggxv + ggxl);
}

vec2 integrateMultiScatteringBRDF(float ndv, float a)
{
    vec3 V;
    V.x = sqrt(1.0 - ndv * ndv);
    V.y = 0.0;
    V.z = ndv;
    vec2 R = vec2(0.0);
    for (uint i = 0; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = hammersley(i, SAMPLE_COUNT);
        vec3 H = importanceSamplingGGX(Xi, a);
        vec3 L = 2.0 * dot(V, H) * H - V;
        float vdh = max(dot(V, H), 0.0);
        float ndl = max(L.z, 0.0);
        float ndh = max(H.z, 0.0);

        if (ndl > 0.0)
        {
            float Vis = V_SmithGGXCorrelated(ndv, ndl, a * a) * ndl * (vdh / ndh);
            float Fc = pow(1.0 - vdh, 5.0);
            /*
             * Assuming f90 = 1
             *   Fc = (1 - V•H)^5
             *   F(h) = f0*(1 - Fc) + Fc
             *
             * f0 and f90 are known at runtime, but thankfully can be factored out, allowing us
             * to split the integral in two terms and store both terms separately in a LUT.
             *
             * At runtime, we can reconstruct Er() exactly as below:
             *
             *            4                <v•h>
             *   DFV.x = --- ∑ Fc V(v, l) ------- <n•l>
             *            N  h             <n•h>
             *
             *
             *            4                <v•h>
             *   DFV.y = --- ∑    V(v, l) ------- <n•l>
             *            N  h             <n•h>
             *
             *
             *   Er() = (1 - f0) * DFV.x + f0 * DFV.y
             *
             *        = mix(DFV.xxx, DFV.yyy, f0)
             *
             */
            R.x += Vis * Fc;
            R.y += Vis;
        }
    }
    return R * (4.0 / SAMPLE_COUNT);
}

void main()
{
    vec2 uv = (gl_GlobalInvocationID.xy + 0.5) / vec2(imageSize(uBRDFLut));
    float ndv = uv.x, a = uv.y * uv.y;
    imageStore(uBRDFLut, ivec2(gl_GlobalInvocationID.xy), vec4(integrateMultiScatteringBRDF(ndv, a), 0.0, 0.0));
}