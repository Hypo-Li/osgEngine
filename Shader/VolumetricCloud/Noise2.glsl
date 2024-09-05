

float hash(float n)
{
    return fract(sin(n + 1.951) * 43758.5453);
}

float noise(const vec3 x)
{
    vec3 p = floor(x);
    vec3 f = fract(x);
    f = f * f * (vec3(3) - vec3(2) * f);
    float n = p.x + p.y * 57.0 + 113.0 * p.z;
    return mix(
        mix(
            mix(hash(n + 0.0), hash(n + 1.0), f.x),
            mix(hash(n + 57.0), hash(n + 58.0), f.x),
            f.y
        ),
        mix(
            mix(hash(n + 113.0), hash(n + 114.0), f.x),
            mix(hash(n + 170.0), hash(n + 171.0), f.x),
            f.y
        ),
        f.z
    );
}

float cells(const vec3 p, float cellCount)
{
    const vec3 pCell = p * cellCount;
    float d = 1.0e10;
    for (int xo = -1; xo <= 1; ++xo)
    {
        for (int yo = -1; yo <= 1; yo++)
		{
			for (int zo = -1; zo <= 1; zo++)
			{
				vec3 tp = floor(pCell) + vec3(xo, yo, zo);

				tp = pCell - tp - noise(mod(tp, cellCount / 1));

				d = min(d, dot(tp, tp));
			}
		}
    }
    d = clamp(d, 0.0, 1.0);
    return d;
}

float worleyNoise(const vec3 p, float cellCount)
{
    return cells(p, cellCount);
}

float perlinNoise(const vec3 pIn, float frequency, int octaveCount)
{
    const float octaveFrenquencyFactor = 2;			// noise frequency factor between octave, forced to 2
    // Compute the sum for each octave
	float sum = 0.0f;
	float weightSum = 0.0f;
	float weight = 0.5f;
	for (int oct = 0; oct < octaveCount; oct++)
	{
		// Perlin vec3 is bugged in GLM on the Z axis :(, black stripes are visible
		// So instead we use 4d Perlin and only use xyz...
		//glm::vec3 p(x * freq, y * freq, z * freq);
		//float val = glm::perlin(p, glm::vec3(freq)) *0.5 + 0.5;

		vec4 p = vec4(pIn.x, pIn.y, pIn.z, 0.0f) * vec4(frequency);
		float val = perlin(p, vec4(frequency));

		sum += val * weight;
		weightSum += weight;

		weight *= weight;
		frequency *= octaveFrenquencyFactor;
	}

	float noise = (sum / weightSum) *0.5f + 0.5f;
	return clamp(noise, 0.0, 1.0);
}