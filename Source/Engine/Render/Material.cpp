#include "Material.h"

static const std::string FrameworkVertexShaderSource = R"(
#version 430 core
in vec4 osg_Vertex;
in vec3 osg_Normal;
in vec4 osg_Color;
in vec4 osg_MultiTexCoord0;
in vec4 osg_MultiTexCoord1;
layout(location = 6) in vec4 osg_Tangent;

out V2F
{
    vec3 fragPosVS;
    vec3 normalWS;
    vec4 tangentWS;
    vec4 color;
    vec4 texcoord0;
    vec4 texcoord1;
} v2f;

uniform mat4 osg_ModelViewMatrix;
uniform mat4 osg_ProjectionMatrix;
uniform mat4 osg_ViewMatrixInverse;
uniform mat3 osg_NormalMatrix;

void main()
{
    vec4 viewSpace = osg_ModelViewMatrix * osg_Vertex;
    gl_Position = osg_ProjectionMatrix * viewSpace;
    v2f.fragPosVS = viewSpace.xyz;
    v2f.normalWS = mat3(osg_ViewMatrixInverse) * osg_NormalMatrix * osg_Normal;
    v2f.tangentWS = vec4(mat3(osg_ViewMatrixInverse) * osg_NormalMatrix * osg_Tangent.xyz, osg_Tangent.w);
    v2f.color = osg_Color;
    v2f.texcoord0 = osg_MultiTexCoord0;
    v2f.texcoord1 = osg_MultiTexCoord1;
}
)";

static const std::string FrameworkFragmentShaderSource = R"(
#version 430 core
#pragma import_defines(SHADING_MODEL)
#pragma import_defines(ALPHA_MODE)

#ifdef SHADING_MODEL
#define UNLIT 0
#define STANDARD 1
#endif

#ifdef ALPHA_MODE
#define OPAQUE 0
#define ALPHA_MASK 1
#define ALPHA_BLEND 2
#endif

in V2F
{
    vec3 fragPosVS;
    vec3 normalWS;
    vec4 tangentWS;
    vec4 color;
    vec4 texcoord0;
    vec4 texcoord1;
} v2f;

#if (ALPHA_MODE != ALPHA_BLEND)
out vec4 fragData[4];
#else
out vec4 fragData;
#endif

struct MaterialInputs
{
    vec3 fragPosVS;
    vec3 normalWS;
    vec3 tangentWS;
    vec3 bitangentWS;
    vec4 color;
    vec4 texcoord0;
    vec4 texcoord1;
};

struct MaterialOutputs
{
    vec3 emissive;
    float opaque;
#if (SHADING_MODEL >= STANDARD)
    vec3 baseColor;
    float metallic;
    float roughness;
    vec3 normal;
    float occlusion;
#endif
};

void calcMaterial(in MaterialInputs mi, out MaterialOutputs mo);

void main()
{
    MaterialInputs mi;
    mi.fragPosVS = v2f.fragPosVS;
    mi.normalWS = normalize(v2f.normalWS);
    mi.tangentWS = normalize(v2f.tangentWS.xyz);
    mi.bitangentWS = normalize(cross(mi.normalWS, mi.tangentWS)) * v2f.tangentWS.w;
    mi.color = v2f.color;
    mi.texcoord0 = v2f.texcoord0;
    mi.texcoord1 = v2f.texcoord1;
    MaterialOutputs mo;
    calcMaterial(mi, mo);

#if (ALPHA_MODE == ALPHA_MASK)
    if (mo.opaque < 0.5)
        discard;
#endif

#if (ALPHA_MODE != ALPHA_BLEND)
    // GBuffer outputs
    fragData[0] = vec4(mo.emissive, 1.0);
    fragData[1] = vec4(mo.normal, 1.0);
    fragData[2] = vec4(mo.baseColor, mo.occlusion);
    fragData[3] = vec4(mo.metallic, mo.roughness, 0.0, 1.0);
#else
    // Forward outputs
    fragData = vec4(0.0);
#endif
}
)";

static const std::string MaterialPreDefines = R"(
#version 430 core
#pragma import_defines(SHADING_MODEL)
#pragma import_defines(ALPHA_MODE)

#ifdef SHADING_MODEL
#define UNLIT 0
#define STANDARD 1
#endif

#ifdef ALPHA_MODE
#define OPAQUE 0
#define ALPHA_MASK 1
#define ALPHA_BLEND 2
#endif

struct MaterialInputs
{
    vec3 fragPosVS;
    vec3 normalWS;
    vec3 tangentWS;
    vec3 bitangentWS;
    vec4 color;
    vec4 texcoord0;
    vec4 texcoord1;
};

struct MaterialOutputs
{
    vec3 emissive;
    float opaque;
#if (SHADING_MODEL >= STANDARD)
    vec3 baseColor;
    float metallic;
    float roughness;
    vec3 normal;
    float occlusion;
#endif
};
)";

static const std::string DefaultMaterialShaderSource = R"(
void calcMaterial(in MaterialInputs mi, out MaterialOutputs mo)
{
    mo.emissive = vec3(0.0);
    mo.opaque = 1.0;
#if (SHADING_MODEL >= STANDARD)
    mo.baseColor = vec3(0.8);
    mo.metallic = 0.0;
    mo.roughness = 0.5;
    mo.normal = mi.normalWS;
    mo.occlusion = 1.0;
#endif
}
)";

namespace xxx
{
    void Material::setShader(Shader* shader)
    {
        if (!shader)
            return;

        mOsgStateSet->clear();

        mShader = shader;

        static osg::ref_ptr<osg::Shader> frameworkVertexShader = new osg::Shader(osg::Shader::VERTEX, FrameworkVertexShaderSource);
        static osg::ref_ptr<osg::Shader> frameworkFragmentShader = new osg::Shader(osg::Shader::FRAGMENT, FrameworkFragmentShaderSource);
        static osg::ref_ptr<osg::Shader> frameworkMaterialShader = new osg::Shader(osg::Shader::FRAGMENT, MaterialPreDefines + DefaultMaterialShaderSource);
        osg::Program* program = new osg::Program;
        program->addShader(frameworkVertexShader);
        program->addShader(frameworkFragmentShader);
        if (mShader->getSource().empty())
        {
            program->addShader(frameworkMaterialShader);
        }
        else
        {
            std::string materialShaderSource = MaterialPreDefines;
            for (auto& parameter : mShader->getParameters())
                materialShaderSource += "uniform " + getParameterTypeString(parameter.second) + " u" + parameter.first + ";\n";
            materialShaderSource += mShader->getSource();
            program->addShader(new osg::Shader(osg::Shader::FRAGMENT, materialShaderSource));
        }
        mOsgStateSet->setAttribute(program, osg::StateAttribute::ON);

        setRenderingPath(mShader->getRenderingPath());
        setShadingModel(mShader->getShadingModel());
        setAlphaMode(mShader->getAlphaMode());
        setDoubleSided(mShader->getDoubleSided());

        int textureUnit = 0;
        const auto& parameters = mShader->getParameters();
        for (const auto& param : parameters)
        {
            std::string uniformName = "u" + param.first;
            osg::Uniform* uniform = nullptr;
            switch (param.second.index())
            {
            case 0:
                uniform = new osg::Uniform(uniformName.c_str(), std::get<bool>(param.second));
                break;
            case 1:
                uniform = new osg::Uniform(uniformName.c_str(), std::get<int>(param.second));
                break;
            case 2:
                uniform = new osg::Uniform(uniformName.c_str(), std::get<float>(param.second));
                break;
            case 3:
                uniform = new osg::Uniform(uniformName.c_str(), std::get<osg::Vec2f>(param.second));
                break;
            case 4:
                uniform = new osg::Uniform(uniformName.c_str(), std::get<osg::Vec3f>(param.second));
                break;
            case 5:
                uniform = new osg::Uniform(uniformName.c_str(), std::get<osg::Vec4f>(param.second));
                break;
            case 6:
            {
                Texture* texture = std::get<osg::ref_ptr<Texture>>(param.second);
                uniform = new osg::Uniform(uniformName.c_str(), textureUnit);
                mOsgStateSet->setTextureAttribute(textureUnit, texture->getOsgTexture(), osg::StateAttribute::ON);
                ++textureUnit;
                break;
            }
            default:
                break;
            }
            mOsgStateSet->addUniform(uniform, osg::StateAttribute::ON);
        }

    }

    void Material::setRenderingPath(RenderingPath renderingPath)
    {

    }

    void Material::setShadingModel(ShadingModel shadingModel)
    {
        mOsgStateSet->setDefine("SHADING_MODEL", std::to_string(int(shadingModel)));
    }

    void Material::setAlphaMode(AlphaMode alphaMode)
    {
        switch (alphaMode)
        {
        case AlphaMode::Opaque:
        case AlphaMode::Mask:
            mOsgStateSet->setMode(GL_BLEND, osg::StateAttribute::OFF);
            mOsgStateSet->setRenderingHint(osg::StateSet::OPAQUE_BIN);
            break;
        case AlphaMode::Blend:
            mOsgStateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
            mOsgStateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
            break;
        default:
            break;
        }
        mOsgStateSet->setDefine("ALPHA_MODE", std::to_string(int(alphaMode)));
    }

    void Material::setDoubleSided(bool doubleSided)
    {
        if (doubleSided)
            mOsgStateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
        else
            mOsgStateSet->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
    }
}