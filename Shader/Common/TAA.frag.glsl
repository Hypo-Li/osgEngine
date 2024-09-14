#version 430 core
in vec2 uv;
out vec4 fragData;
uniform sampler2D uCurrentColorTexture;
uniform sampler2D uHistoryColorTexture;

void main()
{
    vec4 currentColor = textureLod(uCurrentColorTexture, uv, 0.0);
    vec4 historyColor = textureLod(uHistoryColorTexture, uv, 0.0);

    vec2 texelSize = 1.0 / textureSize(uCurrentColorTexture, 0);
    vec4 north = textureLod(uCurrentColorTexture, uv + vec2(0, texelSize.y), 0);
    vec4 south = textureLod(uCurrentColorTexture, uv + vec2(0, -texelSize.y), 0);
    vec4 west = textureLod(uCurrentColorTexture, uv + vec2(-texelSize.x, 0), 0);
    vec4 east = textureLod(uCurrentColorTexture, uv + vec2(texelSize.x, 0), 0);
    vec4 boxMin = min(min(min(north, south), min(east, west)), currentColor);
    vec4 boxMax = max(max(max(north, south), max(east, west)), currentColor);
    historyColor = clamp(historyColor, boxMin, boxMax);

    fragData = mix(historyColor, currentColor, 0.05);
}