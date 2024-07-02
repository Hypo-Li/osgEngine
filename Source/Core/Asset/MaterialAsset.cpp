#include "MaterialAsset.h"

static const char* preDefines = R"(
#pragma import_defines(UNLIT)
#pragma import_defines(STANDARD)
#pragma import_defines(OPAQUE)
#pragma import_defines(ALPHA_MASK)
#pragma import_defines(ALPHA_BLEND)
#pragma import_defines(SHADING_MODEL)
#pragma import_defines(ALPHA_MODE)

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
    const std::unordered_map<GLenum, std::string> MaterialAsset::_sTextureSamplerStringMap = {
        {GL_TEXTURE_2D, "sampler2D"},
        {GL_TEXTURE_2D_ARRAY, "sampler2DArray"},
        {GL_TEXTURE_3D, "sampler3D"},
        {GL_TEXTURE_CUBE_MAP, "samplerCube"},
    };

    MaterialAsset::MaterialAsset() :
        Asset(Type::Material),
        _stateSet(new osg::StateSet),
        _shader(new osg::Shader(osg::Shader::FRAGMENT)),
        _stateSetDirty(false),
        _shaderDirty(false)
    {
        initializeShaderDefines();
        setShadingModel(ShadingModel::Standard);
        setAlphaMode(AlphaMode::Opaque);
        setDoubleSided(false);
        setSource(standardDefaultSource);
        apply();
    }

    void MaterialAsset::serialize(Json& json, std::vector<char>& binary, std::vector<std::string>& reference) const
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
            case static_cast<size_t>(MaterialParameterTypeIndex::Bool):
                parametersJson[parameter.first] = std::get<bool>(parameter.second);
                break;
            case static_cast<size_t>(MaterialParameterTypeIndex::Int):
                parametersJson[parameter.first] = std::get<int>(parameter.second);
                break;
            case static_cast<size_t>(MaterialParameterTypeIndex::Float):
                parametersJson[parameter.first] = std::get<float>(parameter.second);
                break;
            case static_cast<size_t>(MaterialParameterTypeIndex::Float2):
                parametersJson[parameter.first] = osgVec2ToJson(std::get<osg::Vec2>(parameter.second));
                break;
            case static_cast<size_t>(MaterialParameterTypeIndex::Float3):
                parametersJson[parameter.first] = osgVec3ToJson(std::get<osg::Vec3>(parameter.second));
                break;
            case static_cast<size_t>(MaterialParameterTypeIndex::Float4):
                parametersJson[parameter.first] = osgVec4ToJson(std::get<osg::Vec4>(parameter.second));
                break;
            case static_cast<size_t>(MaterialParameterTypeIndex::Texture):
                parametersJson[parameter.first] = "#" + std::to_string(getReferenceIndex(std::get<TextureAssetAndUnit>(parameter.second).first->getPath(), reference));
                break;
            default:
                break;
            }
        }
        json["Parameters"] = parametersJson;
    }

    void MaterialAsset::deserialize(const Json& json, const std::vector<char>& binary, const std::vector<std::string>& reference)
    {
        setShadingModel(static_cast<ShadingModel>(json["ShadingModel"]));
        setAlphaMode(static_cast<AlphaMode>(json["AlphaMode"]));
        setDoubleSided(json["DoubleSided"]);
        setSource(json["Source"]);
        {
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
                    TextureAsset* texture = dynamic_cast<TextureAsset*>(AssetManager::loadAsset(reference[index]));
                    appendParameter(it.key(), texture);
                }
            }
        }
        apply();
    }

    void MaterialAsset::setShadingModel(ShadingModel shadingModel)
    {
        _shadingModel = shadingModel;
        _stateSet->setDefine("SHADING_MODEL", std::to_string(int(shadingModel)));
        _stateSetDirty = true;
    }

    void MaterialAsset::setAlphaMode(AlphaMode alphaMode)
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

    void MaterialAsset::setDoubleSided(bool doubleSided)
    {
        _doubleSided = doubleSided;
        if (_doubleSided)
            _stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
        else
            _stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
        _stateSetDirty = true;
    }

    bool MaterialAsset::removeParameter(const std::string& name)
    {
        if (_parameters.count(name))
        {
            MaterialParameterType& parameter = _parameters[name];
            if (parameter.index() == size_t(MaterialParameterTypeIndex::Texture))
            {
                TextureAssetAndUnit textureAssetAndUnit = std::get<TextureAssetAndUnit>(parameter);
                _stateSet->removeTextureAttribute(textureAssetAndUnit.second, osg::StateAttribute::Type::TEXTURE);
            }
            _stateSet->removeUniform(_stateSet->getUniform(name));
            _parameters.erase(name);
            _uniformLines.erase(name);

            _stateSetDirty = true;
            _shaderDirty = true;
            return true;
        }
        return false;
    }

    void MaterialAsset::setSource(const std::string& source)
    {
        _source = source;
        _shaderDirty = true;
    }

    void MaterialAsset::apply()
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
            _shader->dirtyShader();
            _shaderDirty = false;
        }
        if (_stateSetDirty)
        {
            // SceneMaterialUpdateVisitor smuv(this);
            // sceneRoot->accept(smuv);
            _stateSetDirty = false;
        }
    }

    void MaterialAsset::initializeShaderDefines()
    {
        _stateSet->setDefine("UNLIT", "0");
        _stateSet->setDefine("STANDARD", "1");
        _stateSet->setDefine("OPAQUE", "0");
        _stateSet->setDefine("ALPHA_MASK", "1");
        _stateSet->setDefine("ALPHA_BLEND", "2");
        _stateSetDirty = true;
    }

    int MaterialAsset::getAvailableUnit()
    {
        std::unordered_set<int> unavailableUnit;
        for (const auto& parameter : _parameters)
            if (parameter.second.index() == size_t(MaterialParameterTypeIndex::Texture))
                unavailableUnit.insert(std::get<TextureAssetAndUnit>(parameter.second).second);

        int availableUnit = 0;
        while (unavailableUnit.count(availableUnit))
            ++availableUnit;
        return availableUnit;
    }
}
