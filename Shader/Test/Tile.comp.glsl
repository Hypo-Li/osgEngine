#version 460 core

#define LOCAL_SIZE 16
#define BLOCK_SIZE 2
#define TILE_SIZE_WITH_BORDER (LOCAL_SIZE * BLOCK_SIZE)
#define BORDER_SIZE 1
#define TILE_SIZE (TILE_SIZE_WITH_BORDER - 2 * BORDER_SIZE)

layout (local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE, local_size_z = 1) in;
layout (rgba16f, binding = 0) uniform image2D uInputImage;
layout (rgba16f, binding = 1) uniform image2D uOutputImage;

shared vec4 sharedData[TILE_SIZE_WITH_BORDER][TILE_SIZE_WITH_BORDER];

vec4 processPixel(uint index)
{
    ivec2 threadPixelOffset = ivec2(gl_LocalInvocationID.xy) * BLOCK_SIZE;

    ivec2 sharedPos[4];
    sharedPos[0] = ivec2(threadPixelOffset.x, threadPixelOffset.y);
    sharedPos[1] = ivec2(threadPixelOffset.x + 1, threadPixelOffset.y);
    sharedPos[2] = ivec2(threadPixelOffset.x, threadPixelOffset.y + 1);
    sharedPos[3] = ivec2(threadPixelOffset.x + 1, threadPixelOffset.y + 1);
    ivec2 pos = sharedPos[index];

    const float kernel[9] = {
        0.0625, 0.125, 0.0625,
        0.125, 0.25, 0.125,
        0.0625, 0.125, 0.0625
    };
    vec4 blured =
        sharedData[pos.x - 1][pos.y - 1] * kernel[0] + 
        sharedData[pos.x    ][pos.y - 1] * kernel[1] +
        sharedData[pos.x + 1][pos.y - 1] * kernel[2] +
        sharedData[pos.x - 1][pos.y    ] * kernel[3] +
        sharedData[pos.x    ][pos.y    ] * kernel[4] +
        sharedData[pos.x + 1][pos.y    ] * kernel[5] +
        sharedData[pos.x - 1][pos.y + 1] * kernel[6] +
        sharedData[pos.x    ][pos.y + 1] * kernel[7] +
        sharedData[pos.x + 1][pos.y + 1] * kernel[8];

    return blured;
}

vec4 temporalAASample(vec2 uv)
{
    
}

void main()
{
    ivec2 groupPixelOffset = ivec2(gl_WorkGroupID.xy) * TILE_SIZE - BORDER_SIZE;
    ivec2 threadPixelOffset = ivec2(gl_LocalInvocationID.xy) * BLOCK_SIZE;
    ivec2 pixelOffset[4] = {
        groupPixelOffset + threadPixelOffset,
        groupPixelOffset + threadPixelOffset + ivec2(1, 0),
        groupPixelOffset + threadPixelOffset + ivec2(0, 1),
        groupPixelOffset + threadPixelOffset + ivec2(1, 1)
    };
    ivec2 inputImageSize = imageSize(uInputImage);
    pixelOffset[0] = clamp(pixelOffset[0], ivec2(0), inputImageSize - 1);
    pixelOffset[1] = clamp(pixelOffset[1], ivec2(0), inputImageSize - 1);
    pixelOffset[2] = clamp(pixelOffset[2], ivec2(0), inputImageSize - 1);
    pixelOffset[3] = clamp(pixelOffset[3], ivec2(0), inputImageSize - 1);

    bool isEdge[4];
    isEdge[0] = threadPixelOffset.x == 0 || threadPixelOffset.y == 0;
    isEdge[1] = threadPixelOffset.x == TILE_SIZE || threadPixelOffset.y == 0;
    isEdge[2] = threadPixelOffset.x == 0 || threadPixelOffset.y == TILE_SIZE;
    isEdge[3] = threadPixelOffset.x == TILE_SIZE || threadPixelOffset.y == TILE_SIZE;

    vec4 inputPixel[4];
    inputPixel[0] = imageLoad(uInputImage, pixelOffset[0]);
    inputPixel[1] = imageLoad(uInputImage, pixelOffset[1]);
    inputPixel[2] = imageLoad(uInputImage, pixelOffset[2]);
    inputPixel[3] = imageLoad(uInputImage, pixelOffset[3]);

    ivec2 sharedPos[4];
    sharedPos[0] = ivec2(threadPixelOffset.x, threadPixelOffset.y);
    sharedPos[1] = ivec2(threadPixelOffset.x + 1, threadPixelOffset.y);
    sharedPos[2] = ivec2(threadPixelOffset.x, threadPixelOffset.y + 1);
    sharedPos[3] = ivec2(threadPixelOffset.x + 1, threadPixelOffset.y + 1);

    sharedData[sharedPos[0].x][sharedPos[0].y] = inputPixel[0];
    sharedData[sharedPos[1].x][sharedPos[1].y] = inputPixel[1];
    sharedData[sharedPos[2].x][sharedPos[2].y] = inputPixel[2];
    sharedData[sharedPos[3].x][sharedPos[3].y] = inputPixel[3];

    barrier();

    if (!isEdge[0])
    {
        imageStore(uOutputImage, pixelOffset[0], processPixel(0));
    }
    if (!isEdge[1])
    {
        imageStore(uOutputImage, pixelOffset[1], processPixel(1));
    }
    if (!isEdge[2])
    {
        imageStore(uOutputImage, pixelOffset[2], processPixel(2));
    }
    if (!isEdge[3])
    {
        imageStore(uOutputImage, pixelOffset[3], processPixel(3));
    }
}