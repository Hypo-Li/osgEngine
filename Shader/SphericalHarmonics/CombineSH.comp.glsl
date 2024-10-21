#version 460 core
layout(local_size_x = 128) in;
#define GROUP_NUM 16
layout(std140, binding = 0) buffer SphericalHarmonicsCoefficient
{
    vec3 SHCoeff[9];
};
layout(std140, binding = 1) buffer SphericalHarmonicsTemp
{
    vec3 SHTemp[GROUP_NUM * GROUP_NUM][9];
};
const float PI = 3.14159265358979323846;
const float A0 = PI;
const float A1 = 2.0 * PI / 3.0;
const float A2 = PI / 4.0;

shared vec3 temp[GROUP_NUM * GROUP_NUM / 2][9];

void main()
{
    temp[gl_LocalInvocationIndex][0] = SHTemp[gl_LocalInvocationIndex][0] + SHTemp[gl_LocalInvocationIndex + 128][0];
    temp[gl_LocalInvocationIndex][1] = SHTemp[gl_LocalInvocationIndex][1] + SHTemp[gl_LocalInvocationIndex + 128][1];
    temp[gl_LocalInvocationIndex][2] = SHTemp[gl_LocalInvocationIndex][2] + SHTemp[gl_LocalInvocationIndex + 128][2];
    temp[gl_LocalInvocationIndex][3] = SHTemp[gl_LocalInvocationIndex][3] + SHTemp[gl_LocalInvocationIndex + 128][3];
    temp[gl_LocalInvocationIndex][4] = SHTemp[gl_LocalInvocationIndex][4] + SHTemp[gl_LocalInvocationIndex + 128][4];
    temp[gl_LocalInvocationIndex][5] = SHTemp[gl_LocalInvocationIndex][5] + SHTemp[gl_LocalInvocationIndex + 128][5];
    temp[gl_LocalInvocationIndex][6] = SHTemp[gl_LocalInvocationIndex][6] + SHTemp[gl_LocalInvocationIndex + 128][6];
    temp[gl_LocalInvocationIndex][7] = SHTemp[gl_LocalInvocationIndex][7] + SHTemp[gl_LocalInvocationIndex + 128][7];
    temp[gl_LocalInvocationIndex][8] = SHTemp[gl_LocalInvocationIndex][8] + SHTemp[gl_LocalInvocationIndex + 128][8];
    barrier();

    if (gl_LocalInvocationIndex < 64)
    {
        temp[gl_LocalInvocationIndex][0] += temp[gl_LocalInvocationIndex + 64][0];
        temp[gl_LocalInvocationIndex][1] += temp[gl_LocalInvocationIndex + 64][1];
        temp[gl_LocalInvocationIndex][2] += temp[gl_LocalInvocationIndex + 64][2];
        temp[gl_LocalInvocationIndex][3] += temp[gl_LocalInvocationIndex + 64][3];
        temp[gl_LocalInvocationIndex][4] += temp[gl_LocalInvocationIndex + 64][4];
        temp[gl_LocalInvocationIndex][5] += temp[gl_LocalInvocationIndex + 64][5];
        temp[gl_LocalInvocationIndex][6] += temp[gl_LocalInvocationIndex + 64][6];
        temp[gl_LocalInvocationIndex][7] += temp[gl_LocalInvocationIndex + 64][7];
        temp[gl_LocalInvocationIndex][8] += temp[gl_LocalInvocationIndex + 64][8];
    }
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
        SHCoeff[0] = temp[0][0] + temp[1][0];
        SHCoeff[1] = temp[0][1] + temp[1][1];
        SHCoeff[2] = temp[0][2] + temp[1][2];
        SHCoeff[3] = temp[0][3] + temp[1][3];
        SHCoeff[4] = temp[0][4] + temp[1][4];
        SHCoeff[5] = temp[0][5] + temp[1][5];
        SHCoeff[6] = temp[0][6] + temp[1][6];
        SHCoeff[7] = temp[0][7] + temp[1][7];
        SHCoeff[8] = temp[0][8] + temp[1][8];
    }
}