#include "MaterialAsset.h"
#include <Core/Base/Context.h>
#include <Core/Component/MeshRenderer.h>
#include <osg/NodeVisitor>

static const char* preDefines = R"(
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
    vec4 tangentWS;
    vec4 color;
    vec2 texcoord0;
    vec2 texcoord1;
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

float decodeColor(float data);
vec2 decodeColor(vec2 data);
vec3 decodeColor(vec3 data);
vec4 decodeColor(vec4 data);

float encodeColor(float color);
vec2 encodeColor(vec2 color);
vec3 encodeColor(vec3 color);
vec4 encodeColor(vec4 color);

vec3 decodeNormal(vec3 data);
vec3 encodeNormal(vec3 normal);
)";

static const char* standardDefaultSource = R"(
void calcMaterial(in MaterialInputs mi, out MaterialOutputs mo)
{
    mo.emissive = vec3(0.0);
    mo.opaque = 1.0;
    mo.baseColor = vec3(0.8);
    mo.metallic = 0.0;
    mo.roughness = 0.5;
    mo.normal = vec3(0.5, 0.5, 1.0);
    mo.occlusion = 1.0;
}
)";

namespace xxx
{
    MaterialTemplateAsset::MaterialTemplateAsset() :
        MaterialAsset(Type::MaterialTemplate),
        _stateSet(new osg::StateSet),
        _shader(new osg::Shader(osg::Shader::FRAGMENT)),
        _stateSetDirty(false),
        _shaderDirty(false)
    {
        setShadingModel(ShadingModel::Standard);
        setAlphaMode(AlphaMode::Opaque);
        setDoubleSided(false);
        setSource(standardDefaultSource);
        apply();
    }

    void MaterialTemplateAsset::serialize(Json& json, std::vector<char>& binary, std::vector<std::string>& reference) const
    {
        json["ShadingModel"] = static_cast<int>(_shadingModel);
        json["AlphaMode"] = static_cast<int>(_alphaMode);
        json["DoubleSided"] = _doubleSided;
        json["Source"] = _source;

        Json parametersJson;
        for (const auto& parameter : _parameters)
        {
            switch (parameter.second.index())
            {
            case static_cast<size_t>(ParameterIndex::Bool):
                parametersJson[parameter.first] = std::get<bool>(parameter.second);
                break;
            case static_cast<size_t>(ParameterIndex::Int):
                parametersJson[parameter.first] = std::get<int>(parameter.second);
                break;
            case static_cast<size_t>(ParameterIndex::Float):
                parametersJson[parameter.first] = std::get<float>(parameter.second);
                break;
            case static_cast<size_t>(ParameterIndex::Float2):
                parametersJson[parameter.first] = osgVec2ToJson(std::get<osg::Vec2>(parameter.second));
                break;
            case static_cast<size_t>(ParameterIndex::Float3):
                parametersJson[parameter.first] = osgVec3ToJson(std::get<osg::Vec3>(parameter.second));
                break;
            case static_cast<size_t>(ParameterIndex::Float4):
                parametersJson[parameter.first] = osgVec4ToJson(std::get<osg::Vec4>(parameter.second));
                break;
            case static_cast<size_t>(ParameterIndex::Texture):
                parametersJson[parameter.first] = "#" + std::to_string(getReferenceIndex(std::get<TextureAssetAndUnit>(parameter.second).first->getPath(), reference));
                break;
            default:
                break;
            }
        }
        json["Parameters"] = parametersJson;
    }

    void MaterialTemplateAsset::deserialize(const Json& json, const std::vector<char>& binary, const std::vector<std::string>& reference)
    {
        setShadingModel(static_cast<ShadingModel>(json["ShadingModel"]));
        setAlphaMode(static_cast<AlphaMode>(json["AlphaMode"]));
        setDoubleSided(json["DoubleSided"]);
        setSource(json["Source"]);

        const Json& parametersJson = json["Parameters"];
        for (Json::const_iterator it = parametersJson.begin(); it != parametersJson.end(); it++)
        {
            if (it.value().is_boolean())
                appendParameter(it.key(), it.value().get<bool>());
            else if (it.value().is_number_integer())
                appendParameter(it.key(), it.value().get<int>());
            else if (it.value().is_number_float())
                appendParameter(it.key(), it.value().get<float>());
            else if (it.value().is_array() && it.value().size() == 2)
                appendParameter(it.key(), jsonToOsgVec2(it.value()));
            else if (it.value().is_array() && it.value().size() == 3)
                appendParameter(it.key(), jsonToOsgVec3(it.value()));
            else if (it.value().is_array() && it.value().size() == 4)
                appendParameter(it.key(), jsonToOsgVec4(it.value()));
            else if (it.value().is_string())
            {
                const std::string& path = it.value();
                int index = std::stoi(path.substr(1));
                TextureAsset* texture = AssetManager::loadAsset<TextureAsset>(reference[index]);
                appendParameter(it.key(), texture);
            }
        }

        apply();
    }

    void MaterialTemplateAsset::setShadingModel(ShadingModel shadingModel)
    {
        _shadingModel = shadingModel;
        _stateSetDirty = true;
    }

    void MaterialTemplateAsset::setAlphaMode(AlphaMode alphaMode)
    {
        _alphaMode = alphaMode;
        _stateSetDirty = true;
    }

    void MaterialTemplateAsset::setDoubleSided(bool doubleSided)
    {
        _doubleSided = doubleSided;
        _stateSetDirty = true;
    }

    bool MaterialTemplateAsset::removeParameter(const std::string& name)
    {
        if (_parameters.count(name))
        {
            _parameters.erase(name);
            _stateSetDirty = true;
            _shaderDirty = true;
            return true;
        }
        return false;
    }

    void MaterialTemplateAsset::setSource(const std::string& source)
    {
        _source = source;
        _shaderDirty = true;
    }

    void MaterialTemplateAsset::apply()
    {
        // 将shader与stateset分开apply, 当只有涉及到shader上的修改时, 只需要更新shader而无需更新stateset
        if (_shaderDirty)
        {
            std::string shaderSource = "#version 460 core\n";
            shaderSource += preDefines;
            for (auto& parameter : _parameters)
                shaderSource += "uniform " + getParameterTypeString(parameter.second) + " u" + parameter.first + ";\n";
            shaderSource += _source;
            _shader->setShaderSource(shaderSource);
            _shaderDirty = false;
        }
        if (_stateSetDirty)
        {
            _stateSet->setDefine("SHADING_MODEL", std::to_string(int(_shadingModel)));

            switch (_alphaMode)
            {
            case AlphaMode::Opaque:
            case AlphaMode::Alpha_Mask:
                _stateSet->setMode(GL_BLEND, osg::StateAttribute::OFF);
                _stateSet->setRenderingHint(osg::StateSet::OPAQUE_BIN);
                break;
            case AlphaMode::Alpha_Blend:
                _stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
                _stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
                break;
            default:
                break;
            }
            _stateSet->setDefine("ALPHA_MODE", std::to_string(int(_alphaMode)));

            if (_doubleSided)
                _stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
            else
                _stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::ON);

            _stateSet->getUniformList().clear();
            _stateSet->getTextureAttributeList().clear();

            applyParameters();

            _stateSetDirty = false;
        }
    }

    int MaterialTemplateAsset::getAvailableUnit()
    {
        std::unordered_set<int> unavailableUnit;
        for (const auto& parameter : _parameters)
            if (parameter.second.index() == size_t(ParameterIndex::Texture))
                unavailableUnit.insert(std::get<TextureAssetAndUnit>(parameter.second).second);

        int availableUnit = 0;
        while (unavailableUnit.count(availableUnit))
            ++availableUnit;
        return availableUnit;
    }

    void MaterialTemplateAsset::applyParameters()
    {
        for (auto& parameter : _parameters)
        {
            switch (parameter.second.index())
            {
            case size_t(ParameterIndex::Bool):
                _stateSet->addUniform(new osg::Uniform(("u" + parameter.first).c_str(), std::get<bool>(parameter.second)));
                break;
            case size_t(ParameterIndex::Int):
                _stateSet->addUniform(new osg::Uniform(("u" + parameter.first).c_str(), std::get<int>(parameter.second)));
                break;
            case size_t(ParameterIndex::Float):
                _stateSet->addUniform(new osg::Uniform(("u" + parameter.first).c_str(), std::get<float>(parameter.second)));
                break;
            case size_t(ParameterIndex::Float2):
                _stateSet->addUniform(new osg::Uniform(("u" + parameter.first).c_str(), std::get<osg::Vec2>(parameter.second)));
                break;
            case size_t(ParameterIndex::Float3):
                _stateSet->addUniform(new osg::Uniform(("u" + parameter.first).c_str(), std::get<osg::Vec3>(parameter.second)));
                break;
            case size_t(ParameterIndex::Float4):
                _stateSet->addUniform(new osg::Uniform(("u" + parameter.first).c_str(), std::get<osg::Vec4>(parameter.second)));
                break;
            case size_t(ParameterIndex::Texture):
            {
                TextureAssetAndUnit& textureAssetAndUnit = std::get<TextureAssetAndUnit>(parameter.second);
                _stateSet->addUniform(new osg::Uniform(("u" + parameter.first).c_str(), textureAssetAndUnit.second));
                _stateSet->setTextureAttribute(textureAssetAndUnit.second, textureAssetAndUnit.first->_texture, osg::StateAttribute::ON);
                break;
            }
            default:
                break;
            }
        }
    }

    std::string MaterialTemplateAsset::getParameterTypeString(const Parameter& parameter)
    {
        switch (parameter.index())
        {
        case size_t(ParameterIndex::Bool):
            return "bool";
        case size_t(ParameterIndex::Int):
            return "int";
        case size_t(ParameterIndex::Float):
            return "float";
        case size_t(ParameterIndex::Float2):
            return "vec2";
        case size_t(ParameterIndex::Float3):
            return "vec3";
        case size_t(ParameterIndex::Float4):
            return "vec4";
        case size_t(ParameterIndex::Texture):
        {
            switch (std::get<TextureAssetAndUnit>(parameter).first->_texture->getTextureTarget())
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

//    MaterialInstanceAsset::MaterialInstanceAsset() : MaterialAsset(Type::MaterialInstance) {}
//
//    void MaterialInstanceAsset::serialize(Json& json, std::vector<char>& binary, std::vector<std::string>& reference) const
//    {
//        json["MaterialTemplate"] = "#" + std::to_string(getReferenceIndex(_materialTemplate->getPath(), reference));;
//        Json parametersJson;
//        for (const auto& parameter : _parameters)
//        {
//            Json parameterJson;
//            parameterJson["Enable"] = parameter.second.enable;
//
//            switch (parameter.second.defaultValue.index())
//            {
//            case static_cast<size_t>(ParameterIndex::Bool):
//                parameterJson["Default"] = std::get<bool>(parameter.second.defaultValue);
//                parameterJson["Override"] = std::get<bool>(parameter.second.overrideValue);
//                break;
//            case static_cast<size_t>(ParameterIndex::Int):
//                parameterJson["Default"] = std::get<int>(parameter.second.defaultValue);
//                parameterJson["Override"] = std::get<int>(parameter.second.overrideValue);
//                break;
//            case static_cast<size_t>(ParameterIndex::Float):
//                parameterJson["Default"] = std::get<float>(parameter.second.defaultValue);
//                parameterJson["Override"] = std::get<float>(parameter.second.overrideValue);
//                break;
//            case static_cast<size_t>(ParameterIndex::Float2):
//                parameterJson["Default"] = osgVec2ToJson(std::get<osg::Vec2>(parameter.second.defaultValue));
//                parameterJson["Override"] = osgVec2ToJson(std::get<osg::Vec2>(parameter.second.overrideValue));
//                break;
//            case static_cast<size_t>(ParameterIndex::Float3):
//                parameterJson["Default"] = osgVec3ToJson(std::get<osg::Vec3>(parameter.second.defaultValue));
//                parameterJson["Override"] = osgVec3ToJson(std::get<osg::Vec3>(parameter.second.overrideValue));
//                break;
//            case static_cast<size_t>(ParameterIndex::Float4):
//                parameterJson["Default"] = osgVec4ToJson(std::get<osg::Vec4>(parameter.second.defaultValue));
//                parameterJson["Override"] = osgVec4ToJson(std::get<osg::Vec4>(parameter.second.overrideValue));
//                break;
//            case static_cast<size_t>(ParameterIndex::Texture):
//                parameterJson["Default"] = "#" + std::to_string(getReferenceIndex(std::get<TextureAssetAndUnit>(parameter.second.defaultValue).first->getPath(), reference));
//                parameterJson["Override"] = "#" + std::to_string(getReferenceIndex(std::get<TextureAssetAndUnit>(parameter.second.overrideValue).first->getPath(), reference));
//                break;
//            default:
//                break;
//            }
//
//            parametersJson[parameter.first] = parameterJson;
//        }
//        json["Parameters"] = parametersJson;
//    }
//
//    void MaterialInstanceAsset::deserialize(const Json& json, const std::vector<char>& binary, const std::vector<std::string>& reference)
//    {
//        int index = std::stoi(json["MaterialTemplate"].get<std::string>().substr(1));
//        setMaterialTemplate(AssetManager::loadAsset<MaterialTemplateAsset>(reference[index]));
//
//        const Json& parametersJson = json["Parameters"];
//        for (Json::const_iterator it = parametersJson.begin(); it != parametersJson.end(); it++)
//        {
//            const Json& parameterJson = it.value();
//            InstanceParameter parameter;
//            parameter.enable = parameterJson["Enable"];
//            const Json& defaultValueJson = parameterJson["Default"];
//            const Json& overrideValueJson = parameterJson["Override"];
//
//            if (defaultValueJson.is_boolean())
//            {
//                parameter.defaultValue = defaultValueJson.get<bool>();
//                parameter.overrideValue = overrideValueJson.get<bool>();
//            }
//            else if (defaultValueJson.is_number_integer())
//            {
//                parameter.defaultValue = defaultValueJson.get<int>();
//                parameter.overrideValue = overrideValueJson.get<int>();
//            }
//            else if (defaultValueJson.is_number_float())
//            {
//                parameter.defaultValue = defaultValueJson.get<float>();
//                parameter.overrideValue = overrideValueJson.get<float>();
//            }
//            else if (defaultValueJson.is_array() && defaultValueJson.size() == 2)
//            {
//                parameter.defaultValue = jsonToOsgVec2(defaultValueJson);
//                parameter.overrideValue = jsonToOsgVec2(overrideValueJson);
//            }
//            else if (defaultValueJson.is_array() && defaultValueJson.size() == 3)
//            {
//                parameter.defaultValue = jsonToOsgVec3(defaultValueJson);
//                parameter.overrideValue = jsonToOsgVec3(overrideValueJson);
//            }
//            else if (defaultValueJson.is_array() && defaultValueJson.size() == 4)
//            {
//                parameter.defaultValue = jsonToOsgVec4(defaultValueJson);
//                parameter.overrideValue = jsonToOsgVec4(overrideValueJson);
//            }
//            else if (defaultValueJson.is_string())
//            {
//                TextureAssetAndUnit defaultTextureAssetAndUnit;
//                int defaultTextureIndex = std::stoi(defaultValueJson.get<std::string>().substr(1));
//                defaultTextureAssetAndUnit.first = AssetManager::loadAsset<TextureAsset>(reference[defaultTextureIndex]);
//
//                TextureAssetAndUnit overrideTextureAssetAndUnit;
//                int overrideTextureIndex = std::stoi(overrideValueJson.get<std::string>().substr(1));
//                overrideTextureAssetAndUnit.first = AssetManager::loadAsset<TextureAsset>(reference[overrideTextureIndex]);
//            }
//        }
//    }
//
//    void MaterialInstanceAsset::setMaterialTemplate(MaterialTemplateAsset* materialTemplate)
//    {
//        if (materialTemplate == _materialTemplate)
//            return;
//        _materialTemplate = materialTemplate;
//        _stateSet = new osg::StateSet(*_materialTemplate->getStateSet(), osg::CopyOp::DEEP_COPY_UNIFORMS);
//        _parameters.clear();
//    }
//
//    void MaterialInstanceAsset::syncMaterialTemplate()
//    {
//        osg::ref_ptr<osg::StateSet> newStateSet = new osg::StateSet(*_materialTemplate->getStateSet(), osg::CopyOp::DEEP_COPY_UNIFORMS);
//
//        // 设置已经重载且有效的参数的uniform
//        auto itr = _parameters.begin();
//        while (itr != _parameters.end())
//        {
//            auto findResult = _materialTemplate->_parameters.find(itr->first);
//            if (findResult == _materialTemplate->_parameters.end())
//            {
//                // case 1: 重载参数在新材质模板中不存在
//                _parameters.erase(itr++);
//            }
//            else if (itr->second.index() != findResult->second.index())
//            {
//                // case 2: 重载参数在新材质中存在, 但类型不同
//                _parameters.erase(itr++);
//            }
//            else
//            {
//                // case 3: 重载参数在新材质中存在, 且类型相同
//                std::string uniformName = "u" + itr->first;
//                newStateSet->getUniformList().at(uniformName).first = _stateSet->getUniform(uniformName);
//                itr++;
//            }
//        }
//
//        _stateSet = newStateSet;
//    }
}
