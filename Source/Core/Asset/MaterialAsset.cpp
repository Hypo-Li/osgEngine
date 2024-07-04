#include "MaterialAsset.h"

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
    const std::unordered_map<GLenum, std::string> MaterialTemplateAsset::_sTextureSamplerStringMap = {
        {GL_TEXTURE_2D, "sampler2D"},
        {GL_TEXTURE_2D_ARRAY, "sampler2DArray"},
        {GL_TEXTURE_3D, "sampler3D"},
        {GL_TEXTURE_CUBE_MAP, "samplerCube"},
    };

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
            case static_cast<size_t>(ParameterTypeIndex::Bool):
                parametersJson[parameter.first] = std::get<bool>(parameter.second);
                break;
            case static_cast<size_t>(ParameterTypeIndex::Int):
                parametersJson[parameter.first] = std::get<int>(parameter.second);
                break;
            case static_cast<size_t>(ParameterTypeIndex::Float):
                parametersJson[parameter.first] = std::get<float>(parameter.second);
                break;
            case static_cast<size_t>(ParameterTypeIndex::Float2):
                parametersJson[parameter.first] = osgVec2ToJson(std::get<osg::Vec2>(parameter.second));
                break;
            case static_cast<size_t>(ParameterTypeIndex::Float3):
                parametersJson[parameter.first] = osgVec3ToJson(std::get<osg::Vec3>(parameter.second));
                break;
            case static_cast<size_t>(ParameterTypeIndex::Float4):
                parametersJson[parameter.first] = osgVec4ToJson(std::get<osg::Vec4>(parameter.second));
                break;
            case static_cast<size_t>(ParameterTypeIndex::Texture):
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
        _stateSet->setDefine("SHADING_MODEL", std::to_string(int(shadingModel)));
        _stateSetDirty = true;
    }

    void MaterialTemplateAsset::setAlphaMode(AlphaMode alphaMode)
    {
        _alphaMode = alphaMode;
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
        _stateSet->setDefine("ALPHA_MODE", std::to_string(int(alphaMode)));
        _stateSetDirty = true;
    }

    void MaterialTemplateAsset::setDoubleSided(bool doubleSided)
    {
        _doubleSided = doubleSided;
        if (_doubleSided)
            _stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
        else
            _stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
        _stateSetDirty = true;
    }

    bool MaterialTemplateAsset::removeParameter(const std::string& name)
    {
        if (_parameters.count(name))
        {
            ParameterType& parameter = _parameters[name];
            if (parameter.index() == size_t(ParameterTypeIndex::Texture))
            {
                TextureAssetAndUnit textureAssetAndUnit = std::get<TextureAssetAndUnit>(parameter);
                _stateSet->removeTextureAttribute(textureAssetAndUnit.second, osg::StateAttribute::Type::TEXTURE);
            }
            _stateSet->removeUniform(_stateSet->getUniform("u" + name));
            _parameters.erase(name);
            _uniformLines.erase(name);

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
            for (const auto& uniformLine : _uniformLines)
                shaderSource += uniformLine.second;
            shaderSource += preDefines;
            shaderSource += _source;
            _shader->setShaderSource(shaderSource);
            _shaderDirty = false;
        }
        if (_stateSetDirty)
        {
            // SceneMaterialUpdateVisitor smuv(this);
            // sceneRoot->accept(smuv);
            _stateSetDirty = false;
        }
    }

    int MaterialTemplateAsset::getAvailableUnit()
    {
        std::unordered_set<int> unavailableUnit;
        for (const auto& parameter : _parameters)
            if (parameter.second.index() == size_t(ParameterTypeIndex::Texture))
                unavailableUnit.insert(std::get<TextureAssetAndUnit>(parameter.second).second);

        int availableUnit = 0;
        while (unavailableUnit.count(availableUnit))
            ++availableUnit;
        return availableUnit;
    }

    MaterialInstanceAsset::MaterialInstanceAsset() : MaterialAsset(Type::MaterialInstance) {}

    void MaterialInstanceAsset::serialize(Json& json, std::vector<char>& binary, std::vector<std::string>& reference) const
    {
        json["MaterialTemplate"] = "#" + std::to_string(getReferenceIndex(_materialTemplate->getPath(), reference));;
        Json parametersJson;
        for (const auto& parameter : _parameters)
        {
            switch (parameter.second.index())
            {
            case static_cast<size_t>(ParameterTypeIndex::Bool):
                parametersJson[parameter.first] = std::get<bool>(parameter.second);
                break;
            case static_cast<size_t>(ParameterTypeIndex::Int):
                parametersJson[parameter.first] = std::get<int>(parameter.second);
                break;
            case static_cast<size_t>(ParameterTypeIndex::Float):
                parametersJson[parameter.first] = std::get<float>(parameter.second);
                break;
            case static_cast<size_t>(ParameterTypeIndex::Float2):
                parametersJson[parameter.first] = osgVec2ToJson(std::get<osg::Vec2>(parameter.second));
                break;
            case static_cast<size_t>(ParameterTypeIndex::Float3):
                parametersJson[parameter.first] = osgVec3ToJson(std::get<osg::Vec3>(parameter.second));
                break;
            case static_cast<size_t>(ParameterTypeIndex::Float4):
                parametersJson[parameter.first] = osgVec4ToJson(std::get<osg::Vec4>(parameter.second));
                break;
            case static_cast<size_t>(ParameterTypeIndex::Texture):
                parametersJson[parameter.first] = "#" + std::to_string(getReferenceIndex(std::get<TextureAssetAndUnit>(parameter.second).first->getPath(), reference));
                break;
            default:
                break;
            }
        }
        json["Parameters"] = parametersJson;
    }

    void MaterialInstanceAsset::deserialize(const Json& json, const std::vector<char>& binary, const std::vector<std::string>& reference)
    {
        int index = std::stoi(json["MaterialTemplate"].get<std::string>().substr(1));
        setMaterialTemplate(AssetManager::loadAsset<MaterialTemplateAsset>(reference[index]));

        const Json& parametersJson = json["Parameters"];
        for (Json::const_iterator it = parametersJson.begin(); it != parametersJson.end(); it++)
        {
            if (it.value().is_boolean())
                setParameter(it.key(), it.value().get<bool>());
            else if (it.value().is_number_integer())
                setParameter(it.key(), it.value().get<int>());
            else if (it.value().is_number_float())
                setParameter(it.key(), it.value().get<float>());
            else if (it.value().is_array() && it.value().size() == 2)
                setParameter(it.key(), jsonToOsgVec2(it.value()));
            else if (it.value().is_array() && it.value().size() == 3)
                setParameter(it.key(), jsonToOsgVec3(it.value()));
            else if (it.value().is_array() && it.value().size() == 4)
                setParameter(it.key(), jsonToOsgVec4(it.value()));
            else if (it.value().is_string())
            {
                const std::string& path = it.value();
                int index = std::stoi(path.substr(1));
                TextureAsset* texture = AssetManager::loadAsset<TextureAsset>(reference[index]);
                setParameter(it.key(), texture);
            }
        }
    }

    void MaterialInstanceAsset::setMaterialTemplate(MaterialTemplateAsset* materialTemplate)
    {
        if (materialTemplate == _materialTemplate)
            return;
        _materialTemplate = materialTemplate;
        _stateSet = new osg::StateSet(*_materialTemplate->getStateSet());
        osg::StateSet::UniformList& uniformList = _stateSet->getUniformList();
        // 替换新的uniform对象, 以免影响材质模板
        for (auto& uniform : uniformList)
        {
            osg::ref_ptr<osg::Uniform> newUniform = new osg::Uniform(*uniform.second.first.get());
            uniform.second.first = newUniform;
        }
        _parameters.clear();
    }

    void MaterialInstanceAsset::syncMaterialTemplate()
    {
        osg::ref_ptr<osg::StateSet> newStateSet = new osg::StateSet(*_materialTemplate->getStateSet());
        osg::StateSet::UniformList& newUniformList = newStateSet->getUniformList();

        // 替换新的uniform对象, 以免与影响材质模板
        for (auto& uniform : newUniformList)
        {
            osg::ref_ptr<osg::Uniform> newUniform = new osg::Uniform(*uniform.second.first.get());
            uniform.second.first = newUniform;
        }

        // 设置已经重载且有效的参数的uniform
        auto itr = _parameters.begin();
        while (itr != _parameters.end())
        {
            auto findResult = _materialTemplate->_parameters.find(itr->first);
            if (findResult == _materialTemplate->_parameters.end())
            {
                // case 1: 重载参数在新材质模板中不存在
                _parameters.erase(itr++);
            }
            else if (itr->second.index() != findResult->second.index())
            {
                // case 2: 重载参数在新材质中存在, 但类型不同
                _parameters.erase(itr++);
            }
            else
            {
                // case 3: 重载参数在新材质中存在, 且类型相同
                std::string uniformName = "u" + itr->first;
                newUniformList.at(uniformName).first = _stateSet->getUniform(uniformName);
                itr++;
            }
        }

        _stateSet = newStateSet;
    }
}
