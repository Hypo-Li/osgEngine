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
    vec3 baseColor;
    float metallic;
    float roughness;
    vec3 normal;
    float occlusion;
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
    vec3 baseColor;
    float metallic;
    float roughness;
    vec3 normal;
    float occlusion;
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

        setShadingModel(mShadingModel);
        setAlphaMode(mAlphaMode);
        setDoubleSided(mDoubleSided);

        mShader = shader;
        syncWithShader();
    }

    void Material::syncWithShader()
    {
        static osg::ref_ptr<osg::Shader> frameworkVertexShader = new osg::Shader(osg::Shader::VERTEX, FrameworkVertexShaderSource);
        static osg::ref_ptr<osg::Shader> frameworkFragmentShader = new osg::Shader(osg::Shader::FRAGMENT, FrameworkFragmentShaderSource);
        static osg::ref_ptr<osg::Shader> frameworkMaterialShader = new osg::Shader(osg::Shader::FRAGMENT, MaterialPreDefines + DefaultMaterialShaderSource);

        // create program
        auto& shaderParameters = mShader->getParameters();
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
            for (auto& parameter : shaderParameters)
                materialShaderSource += "uniform " + getParameterTypeString(parameter.second) + " u" + parameter.first + ";\n";
            materialShaderSource += mShader->getSource();
            program->addShader(new osg::Shader(osg::Shader::FRAGMENT, materialShaderSource));
        }
        mOsgStateSet->setAttribute(program, osg::StateAttribute::ON);

        // set parameters and add uniforms
        for (const auto& shaderParam : shaderParameters)
        {
            bool materialParamEnable = false;
            auto materialParamIt = mParameters.find(shaderParam.first);
            if (materialParamIt == mParameters.end() ||
                (materialParamIt != mParameters.end() && materialParamIt->second.first.index() != shaderParam.second.index()))
            {
                mParameters[shaderParam.first] = std::make_pair(shaderParam.second, false);
            }
            else
            {
                materialParamEnable = materialParamIt->second.second;
            }

            std::string uniformName = "u" + shaderParam.first;
            osg::Uniform* uniform = nullptr;
            const Shader::ParameterValue& parameterValue = materialParamEnable ? materialParamIt->second.first : shaderParam.second;
            switch (parameterValue.index())
            {
            case size_t(Shader::ParameterIndex::Bool):
                uniform = new osg::Uniform(uniformName.c_str(), std::get<bool>(parameterValue));
                break;
            case size_t(Shader::ParameterIndex::Int):
                uniform = new osg::Uniform(uniformName.c_str(), std::get<int>(parameterValue));
                break;
            case size_t(Shader::ParameterIndex::Float):
                uniform = new osg::Uniform(uniformName.c_str(), std::get<float>(parameterValue));
                break;
            case size_t(Shader::ParameterIndex::Vec2f):
                uniform = new osg::Uniform(uniformName.c_str(), std::get<osg::Vec2f>(parameterValue));
                break;
            case size_t(Shader::ParameterIndex::Vec3f):
                uniform = new osg::Uniform(uniformName.c_str(), std::get<osg::Vec3f>(parameterValue));
                break;
            case size_t(Shader::ParameterIndex::Vec4f):
                uniform = new osg::Uniform(uniformName.c_str(), std::get<osg::Vec4f>(parameterValue));
                break;
            case size_t(Shader::ParameterIndex::Texture):
            {
                const Shader::TextureAndUnit& textureAndUnit = std::get<Shader::TextureAndUnit>(parameterValue);
                uniform = new osg::Uniform(uniformName.c_str(), textureAndUnit.second);
                mOsgStateSet->setTextureAttribute(textureAndUnit.second, textureAndUnit.first->getOsgTexture(), osg::StateAttribute::ON);
                break;
            }
            default:
                break;
            }
            mOsgStateSet->addUniform(uniform, osg::StateAttribute::ON);
        }
    }

    void Material::setShadingModel(ShadingModel shadingModel)
    {
        mShadingModel = shadingModel;
        mOsgStateSet->setDefine("SHADING_MODEL", std::to_string(int(mShadingModel)));
    }

    void Material::setAlphaMode(AlphaMode alphaMode)
    {
        mAlphaMode = alphaMode;
        switch (mAlphaMode)
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
        mOsgStateSet->setDefine("ALPHA_MODE", std::to_string(int(mAlphaMode)));
    }

    void Material::setDoubleSided(bool doubleSided)
    {
        mDoubleSided = doubleSided;
        if (mDoubleSided)
            mOsgStateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
        else
            mOsgStateSet->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
    }

    std::string Material::getParameterTypeString(const Shader::ParameterValue& parameter)
    {
        switch (parameter.index())
        {
        case size_t(Shader::ParameterIndex::Bool):
            return "bool";
        case size_t(Shader::ParameterIndex::Int):
            return "int";
        case size_t(Shader::ParameterIndex::Float):
            return "float";
        case size_t(Shader::ParameterIndex::Vec2f):
            return "vec2";
        case size_t(Shader::ParameterIndex::Vec3f):
            return "vec3";
        case size_t(Shader::ParameterIndex::Vec4f):
            return "vec4";
        case size_t(Shader::ParameterIndex::Texture):
        {
            switch (std::get<Shader::TextureAndUnit>(parameter).first->getOsgTexture()->getTextureTarget())
            {
            case GL_TEXTURE_2D:
                return "sampler2D";
            case GL_TEXTURE_2D_ARRAY:
                return "sampler2DArray";
            case GL_TEXTURE_3D:
                return "sampler3D";
            case GL_TEXTURE_CUBE_MAP:
                return "samplerCube";
            default:
                return "";
            }
        }
        default:
            return "";
        }
    }
}
