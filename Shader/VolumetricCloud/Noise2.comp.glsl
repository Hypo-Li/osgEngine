#version 430 core
layout(local_size_x = 4, local_size_y = 4, local_size_z = 32) in;
layout(rgba8, binding = 0) uniform image3D uNoise2Image;

// Hash by David_Hoskins
#define UI0 1597334673U
#define UI1 3812015801U
#define UI2 uvec2(UI0, UI1)
#define UI3 uvec3(UI0, UI1, 2798796415U)
#define UIF (1.0 / float(0xffffffffU))

vec3 hash33(vec3 p)
{
    uvec3 q = uvec3(ivec3(p)) * UI3;
    q = (q.x ^ q.y ^ q.z) * UI3;
    return -1. + 2. * vec3(q) * UIF;
}

float remap(float x, float a, float b, float c, float d)
{
    return (((x - a) / (b - a)) * (d - c)) + c;
}

// Tileable 3D inverted worley noise
float worleyNoise(vec3 uv, float freq)
{    
    vec3 id = floor(uv);
    vec3 p = fract(uv);
    
    float minDist = 10000.;
    for (float x = -1.; x <= 1.; ++x)
    {
        for(float y = -1.; y <= 1.; ++y)
        {
            for(float z = -1.; z <= 1.; ++z)
            {
                vec3 offset = vec3(x, y, z);
            	vec3 h = hash33(mod(id + offset, vec3(freq))) * .5 + .5;
    			h += offset;
            	vec3 d = p - h;
           		minDist = min(minDist, dot(d, d));
            }
        }
    }
    
    // inverted worley noise
    return 1. - minDist;
}

// Tileable Worley fbm inspired by Andrew Schneider's Real-Time Volumetric Cloudscapes
// chapter in GPU Pro 7.
float worleyFbm(vec3 p, float freq)
{
    return worleyNoise(p*freq, freq) * 0.625 +
        	worleyNoise(p*freq*2., freq*2.) * 0.250 +
        	worleyNoise(p*freq*4., freq*4.) * 0.125;
}

void main()
{
    const float freq = 8.0;
    vec3 uvw = (gl_GlobalInvocationID.xyz + 0.5) / vec3(32);
    vec3 wfbm = vec3(
        worleyFbm(uvw, freq),
        worleyFbm(uvw, freq * 2.0),
        worleyFbm(uvw, freq * 4.0)
    );
    imageStore(uNoise2Image, ivec3(gl_GlobalInvocationID.xyz), vec4(wfbm, 1.0));
    //imageStore(uNoise2Image, ivec3(gl_GlobalInvocationID.xyz), vec4(dot(wfbm, vec3(0.625, 0.250, 0.125))));
}