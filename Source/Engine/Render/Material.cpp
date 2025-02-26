#include "Material.h"

static const std::string MaterialPreDefines =
R"(#version 430 core
struct MaterialInputs
{
    vec3 fragPosVS;
    vec3 normalVS;
    vec3 normalWS;
    vec3 tangentVS;
    vec3 tangentWS;
    vec4 color;
    vec4 texcoord0;
    vec4 texcoord1;
    vec3 viewDirWS;
    vec3 reflDirWS;
};

struct MaterialOutputs
{
    vec3 emissive;
    float opacity;
    vec3 baseColor;
    float metallic;
    float roughness;
    float specular;
    vec3 normal;
    float occlusion;
};

)";

static const std::string DefaultMaterialShaderSource = R"(
void calcMaterial(in MaterialInputs mi, inout MaterialOutputs mo) {}
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
        static osg::ref_ptr<osg::Shader> frameworkVertexShader = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Test/Framework.vert.glsl");
        static osg::ref_ptr<osg::Shader> frameworkFragmentShader = osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Test/Framework.frag.glsl");
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

        std::set<std::string> shaderParamNames;
        // set parameters and add uniforms
        for (const auto& shaderParam : shaderParameters)
        {
            shaderParamNames.insert(shaderParam.first);

            bool materialParamEnable = false;
            auto materialParamIt = mParameters.find(shaderParam.first);
            if (materialParamIt == mParameters.end() ||
                (materialParamIt != mParameters.end() && materialParamIt->second.first.index() != shaderParam.second.index()))
            {
                // 原材质参数中没有这个参数 或 有该参数但类型不同
                mParameters[shaderParam.first] = std::make_pair(shaderParam.second, false);
            }
            else
            {
                // 继承原材质参数的是否应用属性
                materialParamEnable = materialParamIt->second.second;
                // 如果未应用, 更新默认值
                if (!materialParamEnable)
                    materialParamIt->second.first = shaderParam.second;
            }

            std::string uniformName = "u" + shaderParam.first;
            osg::Uniform* uniform = nullptr;
            const Shader::ParameterValue& parameterValue = materialParamEnable ? materialParamIt->second.first : shaderParam.second;
            switch (parameterValue.index())
            {
            case size_t(Shader::ParameterType::Bool):
                uniform = new osg::Uniform(uniformName.c_str(), std::get<bool>(parameterValue));
                break;
            case size_t(Shader::ParameterType::Int):
                uniform = new osg::Uniform(uniformName.c_str(), std::get<int>(parameterValue));
                break;
            case size_t(Shader::ParameterType::Float):
                uniform = new osg::Uniform(uniformName.c_str(), std::get<float>(parameterValue));
                break;
            case size_t(Shader::ParameterType::Vec2f):
                uniform = new osg::Uniform(uniformName.c_str(), std::get<osg::Vec2f>(parameterValue));
                break;
            case size_t(Shader::ParameterType::Vec3f):
                uniform = new osg::Uniform(uniformName.c_str(), std::get<osg::Vec3f>(parameterValue));
                break;
            case size_t(Shader::ParameterType::Vec4f):
                uniform = new osg::Uniform(uniformName.c_str(), std::get<osg::Vec4f>(parameterValue));
                break;
            case size_t(Shader::ParameterType::Texture2D):
            {
                const Shader::Texture2DUnitPair& textureAndUnit = std::get<Shader::Texture2DUnitPair>(parameterValue);
                uniform = new osg::Uniform(uniformName.c_str(), textureAndUnit.second);
                mOsgStateSet->setTextureAttribute(textureAndUnit.second, textureAndUnit.first->getOsgTexture(), osg::StateAttribute::ON);
                break;
            }
            case size_t(Shader::ParameterType::Texture2DArray):
            {
                const Shader::Texture2DArrayUnitPair& textureAndUnit = std::get<Shader::Texture2DArrayUnitPair>(parameterValue);
                uniform = new osg::Uniform(uniformName.c_str(), textureAndUnit.second);
                mOsgStateSet->setTextureAttribute(textureAndUnit.second, textureAndUnit.first->getOsgTexture(), osg::StateAttribute::ON);
                break;
            }
            case size_t(Shader::ParameterType::Texture3D):
            {
                const Shader::Texture3DUnitPair& textureAndUnit = std::get<Shader::Texture3DUnitPair>(parameterValue);
                uniform = new osg::Uniform(uniformName.c_str(), textureAndUnit.second);
                mOsgStateSet->setTextureAttribute(textureAndUnit.second, textureAndUnit.first->getOsgTexture(), osg::StateAttribute::ON);
                break;
            }
            case size_t(Shader::ParameterType::TextureCubemap):
            {
                const Shader::TextureCubemapUnitPair& textureAndUnit = std::get<Shader::TextureCubemapUnitPair>(parameterValue);
                uniform = new osg::Uniform(uniformName.c_str(), textureAndUnit.second);
                mOsgStateSet->setTextureAttribute(textureAndUnit.second, textureAndUnit.first->getOsgTexture(), osg::StateAttribute::ON);
                break;
            }
            default:
                break;
            }
            mOsgStateSet->addUniform(uniform, osg::StateAttribute::ON);
        }

        // remove useless parameter
        for (auto materialParamIt = mParameters.begin(); materialParamIt != mParameters.end();)
        {
            if (!shaderParamNames.count(materialParamIt->first))
                materialParamIt = mParameters.erase(materialParamIt);
            else
                ++materialParamIt;
        }
    }

    void Material::setShadingModel(ShadingModel shadingModel)
    {
        mShadingModel = shadingModel;
        mOsgStateSet->setDefine("SHADING_MODEL", std::to_string(int(mShadingModel)));

        // TODO: apply material modify in MeshRenderer
        uint32_t osgNodeMask = getOsgNodeMask();
        for (osg::Node* geom : mOsgStateSet->getParents())
            geom->setNodeMask(osgNodeMask);
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

        uint32_t osgNodeMask = getOsgNodeMask();
        for (osg::Node* geom : mOsgStateSet->getParents())
            geom->setNodeMask(osgNodeMask);
    }

    void Material::setDoubleSided(bool doubleSided)
    {
        mDoubleSided = doubleSided;
        if (mDoubleSided)
            mOsgStateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
        else
            mOsgStateSet->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
    }

    uint32_t Material::getOsgNodeMask()
    {
        if (mAlphaMode == AlphaMode::Blend)
        {
            if (mShadingModel == ShadingModel::Unlit)
                return POST_FORWARD_MASK;
            else
                return TRANSPARENT_MASK;
        }
        else
        {
            if (mShadingModel == ShadingModel::Unlit)
                return POST_FORWARD_MASK;
            else
                return GBUFFER_MASK | SHADOW_CAST_MASK;
        }
    }

    std::string Material::getParameterTypeString(const Shader::ParameterValue& parameter)
    {
        switch (parameter.index())
        {
        case size_t(Shader::ParameterType::Bool):
            return "bool";
        case size_t(Shader::ParameterType::Int):
            return "int";
        case size_t(Shader::ParameterType::Float):
            return "float";
        case size_t(Shader::ParameterType::Vec2f):
            return "vec2";
        case size_t(Shader::ParameterType::Vec3f):
            return "vec3";
        case size_t(Shader::ParameterType::Vec4f):
            return "vec4";
        case size_t(Shader::ParameterType::Texture2D):
            return "sampler2D";
        case size_t(Shader::ParameterType::Texture2DArray):
            return "sampler2DArray";
        case size_t(Shader::ParameterType::Texture3D):
            return "sampler3D";
        case size_t(Shader::ParameterType::TextureCubemap):
            return "samplerCube";
        default:
            return "";
        }
    }

    void Material::applyParameter(Parameters::const_iterator materialParamIt)
    {
        std::string uniformName = "u" + materialParamIt->first;
        osg::Uniform* uniform = mOsgStateSet->getUniform(uniformName);

        bool materialParamEnable = materialParamIt->second.second;
        auto shaderParamIt = mShader->getParameters().find(materialParamIt->first);

        const Shader::ParameterValue& parameterValue = materialParamEnable ? materialParamIt->second.first : shaderParamIt->second;

        switch (materialParamIt->second.first.index())
        {
        case size_t(Shader::ParameterType::Bool):
            uniform->set(std::get<bool>(parameterValue));
            return;
        case size_t(Shader::ParameterType::Int):
            uniform->set(std::get<int>(parameterValue));
            return;
        case size_t(Shader::ParameterType::Float):
            uniform->set(std::get<float>(parameterValue));
            return;
        case size_t(Shader::ParameterType::Vec2f):
            uniform->set(std::get<osg::Vec2f>(parameterValue));
            return;
        case size_t(Shader::ParameterType::Vec3f):
            uniform->set(std::get<osg::Vec3f>(parameterValue));
            return;
        case size_t(Shader::ParameterType::Vec4f):
            uniform->set(std::get<osg::Vec4f>(parameterValue));
            return;
        case size_t(Shader::ParameterType::Texture2D):
        {
            const Shader::Texture2DUnitPair& textureAndUnit = std::get<Shader::Texture2DUnitPair>(parameterValue);
            mOsgStateSet->setTextureAttribute(textureAndUnit.second, textureAndUnit.first->getOsgTexture(), osg::StateAttribute::ON);
            return;
        }
        case size_t(Shader::ParameterType::Texture2DArray):
        {
            const Shader::Texture2DArrayUnitPair& textureAndUnit = std::get<Shader::Texture2DArrayUnitPair>(parameterValue);
            mOsgStateSet->setTextureAttribute(textureAndUnit.second, textureAndUnit.first->getOsgTexture(), osg::StateAttribute::ON);
            return;
        }
        case size_t(Shader::ParameterType::Texture3D):
        {
            const Shader::Texture3DUnitPair& textureAndUnit = std::get<Shader::Texture3DUnitPair>(parameterValue);
            mOsgStateSet->setTextureAttribute(textureAndUnit.second, textureAndUnit.first->getOsgTexture(), osg::StateAttribute::ON);
            return;
        }
        case size_t(Shader::ParameterType::TextureCubemap):
        {
            const Shader::TextureCubemapUnitPair& textureAndUnit = std::get<Shader::TextureCubemapUnitPair>(parameterValue);
            mOsgStateSet->setTextureAttribute(textureAndUnit.second, textureAndUnit.first->getOsgTexture(), osg::StateAttribute::ON);
            return;
        }
        default:
            return;
        }
    }
}
