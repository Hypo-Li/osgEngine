#version 460 core
layout(local_size_x = 32, local_size_y = 32) in;
layout(rgba16f, binding = 0) uniform image2D uCloudMapImage;

float distanceToEdge(ivec2 coord, ivec2 imageSize) {
    float minDist = float(imageSize.x * imageSize.y);
    for (int y = 0; y < imageSize.y; ++y) {
        for (int x = 0; x < imageSize.x; ++x) {
            ivec2 currentCoord = ivec2(x, y);
            vec4 pixel = imageLoad(uCloudMapImage, currentCoord);
            if (pixel.r > 0.0) { // Assuming black pixels are the object
                float dist = distance(vec2(coord), vec2(currentCoord));
                minDist = min(minDist, dist);
            }
        }
    }
    return minDist;
}

void main()
{
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imageSize = imageSize(uCloudMapImage);
    float dist = distanceToEdge(coord, imageSize);
    vec3 cloudMapRGB = imageLoad(uCloudMapImage, coord).rgb;
    float sdfValue = dist / float(max(imageSize.x, imageSize.y));
    imageStore(uCloudMapImage, coord, vec4(cloudMapRGB, sdfValue));
}