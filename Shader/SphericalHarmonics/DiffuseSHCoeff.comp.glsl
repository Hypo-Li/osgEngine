#version 460 core
layout(local_size_x = 8, local_size_y = 8) in;
#define GROUP_NUM 16
layout(std140, binding = 0) buffer SphericalHarmonicsCoeff
{
    vec3 SHCoeff[GROUP_NUM * GROUP_NUM][9];
};
uniform samplerCube uCubemapTexture;
const float PI = 3.14159265358979323846;
const float A0 = PI;
const float A1 = 2.0 * PI / 3.0;
const float A2 = PI / 4.0;

void getBasis(vec3 n, out float basis[9])
{
    float x = n.x, y = n.y, z = n.z;
    basis[0] = 0.5 * sqrt(1.0 / PI);

    basis[1] = -0.5 * sqrt(3.0 / PI) * y;
    basis[2] = 0.5 * sqrt(3.0 / PI) * z;
    basis[3] = -0.5 * sqrt(3.0 / PI) * x;

    basis[4] = 0.5 * sqrt(15.0 / PI) * x * y;
    basis[5] = -0.5 * sqrt(15.0 / PI) * y * z;
    basis[6] = 0.25 * sqrt(5.0 / PI) * (3.0 * z * z - 1.0);
    basis[7] = -0.5 * sqrt(15.0 / PI) * x * z;
    basis[8] = 0.25 * sqrt(15.0 / PI) * (x * x - y * y);
}

vec2 sampleSize = vec2(GROUP_NUM * 8, GROUP_NUM * 8);

vec3 getSamplingVector()
{
    float theta = acos(1.0 - gl_GlobalInvocationID.x * 2.0 / (sampleSize.x - 1.0)); // 0-π
    float phi = 2.0 * PI * (gl_GlobalInvocationID.y / (sampleSize.y - 1.0)); // 0-2π
    return vec3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));
}

shared vec3 temp[64][9];

void main()
{
    vec3 v = getSamplingVector();
    vec3 color = texture(uCubemapTexture, v).rgb;
    // pre divide PI cause diffuse need to divide it
    float A = 4 / (sampleSize.x * sampleSize.y);//4 * PI / (sampleSize.x * sampleSize.y);
    float basis[9];
    getBasis(v, basis);
    temp[gl_LocalInvocationIndex][0] = color * basis[0] * A * A0;
    temp[gl_LocalInvocationIndex][1] = color * basis[1] * A * A1;
    temp[gl_LocalInvocationIndex][2] = color * basis[2] * A * A1;
    temp[gl_LocalInvocationIndex][3] = color * basis[3] * A * A1;
    temp[gl_LocalInvocationIndex][4] = color * basis[4] * A * A2;
    temp[gl_LocalInvocationIndex][5] = color * basis[5] * A * A2;
    temp[gl_LocalInvocationIndex][6] = color * basis[6] * A * A2;
    temp[gl_LocalInvocationIndex][7] = color * basis[7] * A * A2;
    temp[gl_LocalInvocationIndex][8] = color * basis[8] * A * A2;
    barrier();

    if (gl_LocalInvocationIndex < 32)
    {
        temp[gl_LocalInvocationIndex][0] += temp[gl_LocalInvocationIndex + 32][0];
        temp[gl_LocalInvocationIndex][1] += temp[gl_LocalInvocationIndex + 32][1];
        temp[gl_LocalInvocationIndex][2] += temp[gl_LocalInvocationIndex + 32][2];
        temp[gl_LocalInvocationIndex][3] += temp[gl_LocalInvocationIndex + 32][3];
        temp[gl_LocalInvocationIndex][4] += temp[gl_LocalInvocationIndex + 32][4];
        temp[gl_LocalInvocationIndex][5] += temp[gl_LocalInvocationIndex + 32][5];
        temp[gl_LocalInvocationIndex][6] += temp[gl_LocalInvocationIndex + 32][6];
        temp[gl_LocalInvocationIndex][7] += temp[gl_LocalInvocationIndex + 32][7];
        temp[gl_LocalInvocationIndex][8] += temp[gl_LocalInvocationIndex + 32][8];
    }
    barrier();

    if (gl_LocalInvocationIndex < 16)
    {
        temp[gl_LocalInvocationIndex][0] += temp[gl_LocalInvocationIndex + 16][0];
        temp[gl_LocalInvocationIndex][1] += temp[gl_LocalInvocationIndex + 16][1];
        temp[gl_LocalInvocationIndex][2] += temp[gl_LocalInvocationIndex + 16][2];
        temp[gl_LocalInvocationIndex][3] += temp[gl_LocalInvocationIndex + 16][3];
        temp[gl_LocalInvocationIndex][4] += temp[gl_LocalInvocationIndex + 16][4];
        temp[gl_LocalInvocationIndex][5] += temp[gl_LocalInvocationIndex + 16][5];
        temp[gl_LocalInvocationIndex][6] += temp[gl_LocalInvocationIndex + 16][6];
        temp[gl_LocalInvocationIndex][7] += temp[gl_LocalInvocationIndex + 16][7];
        temp[gl_LocalInvocationIndex][8] += temp[gl_LocalInvocationIndex + 16][8];
    }
    barrier();

    if (gl_LocalInvocationIndex < 8)
    {
        temp[gl_LocalInvocationIndex][0] += temp[gl_LocalInvocationIndex + 8][0];
        temp[gl_LocalInvocationIndex][1] += temp[gl_LocalInvocationIndex + 8][1];
        temp[gl_LocalInvocationIndex][2] += temp[gl_LocalInvocationIndex + 8][2];
        temp[gl_LocalInvocationIndex][3] += temp[gl_LocalInvocationIndex + 8][3];
        temp[gl_LocalInvocationIndex][4] += temp[gl_LocalInvocationIndex + 8][4];
        temp[gl_LocalInvocationIndex][5] += temp[gl_LocalInvocationIndex + 8][5];
        temp[gl_LocalInvocationIndex][6] += temp[gl_LocalInvocationIndex + 8][6];
        temp[gl_LocalInvocationIndex][7] += temp[gl_LocalInvocationIndex + 8][7];
        temp[gl_LocalInvocationIndex][8] += temp[gl_LocalInvocationIndex + 8][8];
    }
    barrier();

    if (gl_LocalInvocationIndex < 4)
    {
        temp[gl_LocalInvocationIndex][0] += temp[gl_LocalInvocationIndex + 4][0];
        temp[gl_LocalInvocationIndex][1] += temp[gl_LocalInvocationIndex + 4][1];
        temp[gl_LocalInvocationIndex][2] += temp[gl_LocalInvocationIndex + 4][2];
        temp[gl_LocalInvocationIndex][3] += temp[gl_LocalInvocationIndex + 4][3];
        temp[gl_LocalInvocationIndex][4] += temp[gl_LocalInvocationIndex + 4][4];
        temp[gl_LocalInvocationIndex][5] += temp[gl_LocalInvocationIndex + 4][5];
        temp[gl_LocalInvocationIndex][6] += temp[gl_LocalInvocationIndex + 4][6];
        temp[gl_LocalInvocationIndex][7] += temp[gl_LocalInvocationIndex + 4][7];
        temp[gl_LocalInvocationIndex][8] += temp[gl_LocalInvocationIndex + 4][8];
    }
    barrier();

    if (gl_LocalInvocationIndex < 2)
    {
        temp[gl_LocalInvocationIndex][0] += temp[gl_LocalInvocationIndex + 2][0];
        temp[gl_LocalInvocationIndex][1] += temp[gl_LocalInvocationIndex + 2][1];
        temp[gl_LocalInvocationIndex][2] += temp[gl_LocalInvocationIndex + 2][2];
        temp[gl_LocalInvocationIndex][3] += temp[gl_LocalInvocationIndex + 2][3];
        temp[gl_LocalInvocationIndex][4] += temp[gl_LocalInvocationIndex + 2][4];
        temp[gl_LocalInvocationIndex][5] += temp[gl_LocalInvocationIndex + 2][5];
        temp[gl_LocalInvocationIndex][6] += temp[gl_LocalInvocationIndex + 2][6];
        temp[gl_LocalInvocationIndex][7] += temp[gl_LocalInvocationIndex + 2][7];
        temp[gl_LocalInvocationIndex][8] += temp[gl_LocalInvocationIndex + 2][8];
    }
    barrier();

    if (gl_LocalInvocationIndex < 1)
    {
        SHCoeff[gl_WorkGroupID.x * GROUP_NUM + gl_WorkGroupID.y][0] = temp[0][0] + temp[1][0];
        SHCoeff[gl_WorkGroupID.x * GROUP_NUM + gl_WorkGroupID.y][1] = temp[0][1] + temp[1][1];
        SHCoeff[gl_WorkGroupID.x * GROUP_NUM + gl_WorkGroupID.y][2] = temp[0][2] + temp[1][2];
        SHCoeff[gl_WorkGroupID.x * GROUP_NUM + gl_WorkGroupID.y][3] = temp[0][3] + temp[1][3];
        SHCoeff[gl_WorkGroupID.x * GROUP_NUM + gl_WorkGroupID.y][4] = temp[0][4] + temp[1][4];
        SHCoeff[gl_WorkGroupID.x * GROUP_NUM + gl_WorkGroupID.y][5] = temp[0][5] + temp[1][5];
        SHCoeff[gl_WorkGroupID.x * GROUP_NUM + gl_WorkGroupID.y][6] = temp[0][6] + temp[1][6];
        SHCoeff[gl_WorkGroupID.x * GROUP_NUM + gl_WorkGroupID.y][7] = temp[0][7] + temp[1][7];
        SHCoeff[gl_WorkGroupID.x * GROUP_NUM + gl_WorkGroupID.y][8] = temp[0][8] + temp[1][8];
    }
    barrier();
}