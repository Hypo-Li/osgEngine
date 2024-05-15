#include "Material.h"

namespace xxx
{
    const std::string_view Material::_sUnlitMaterialDefaultShaderSource =
R"(
struct UnlitParameters
{
    vec3 color;
    float alpha;
}

void getUnlitParameters(out UnlitParameters parameters)
{
    parameters.color = vec3(0.8);
    parameters.alpha = 1.0;
}
)";

    const std::string_view Material::_sStandardMaterialDefaultShaderSource =
R"(
struct StandardParameters
{
    vec3 baseColor;
    float metallic;
    float roughness;
    float alpha;
    vec3 normal;
    vec3 emissive;
    float occlusion;
}

void getStandardParameters(out StandardParameters parameters)
{
    parameters.baseColor = vec3(0.8);
    parameters.metallic = 0.0;
    parameters.roughness = 0.5;
    parameters.alpha = 1.0;
    parameters.normal = vec3(0.5, 0.5, 1.0);
    parameters.emissive = vec3(0.0);
    parameters.occlusion = 1.0;
}
)";
}
