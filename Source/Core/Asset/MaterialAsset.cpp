#include "MaterialAsset.h"

namespace xxx
{
    MaterialAsset::MaterialAsset() : Asset(Type::Material), _shader(new osg::Shader(osg::Shader::FRAGMENT))
    {
        initializeShaderDefines();
        setShadingModel(ShadingModel::Standard);
        setAlphaBlend(AlphaMode::Opaque);
        setDoubleSided(false);
    }

    void MaterialAsset::serialize(Json& json, std::vector<char>& binray, std::vector<std::string>& reference) const
    {
        json["ShadingModel"] = static_cast<int>(_shadingModel);
        json["AlphaMode"] = static_cast<int>(_alphaMode);
        json["DoubleSided"] = _doubleSided;

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
                parametersJson[parameter.first] = "#" + std::to_string(getReferenceIndex(std::get<TextureAndUnit>(parameter.second).first->getPath(), reference));
                break;
            default:
                break;
            }
        }
        json["Parameters"] = parametersJson;
    }

    void MaterialAsset::deserialize(const Json& json, const std::vector<char>& binray, const std::vector<std::string>& reference)
    {
        if (json.contains("ShadingModel"))
            setShadingModel(static_cast<ShadingModel>(json["ShadingModel"].get<int>()));
        if (json.contains("AlphaMode"))
            setAlphaBlend(static_cast<AlphaMode>(json["AlphaMode"].get<int>()));
        if (json.contains("DoubleSided"))
            setDoubleSided(json["DoubleSided"].get<bool>());
        if (json.contains("Parameters"))
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
    }

    void MaterialAsset::setShadingModel(ShadingModel shadingModel)
    {
        _shadingModel = shadingModel;
        _stateSet->setDefine("SHADING_MODEL", std::to_string(int(shadingModel)));
    }

    void MaterialAsset::setAlphaBlend(AlphaMode alphaMode)
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
    }

    void MaterialAsset::setDoubleSided(bool doubleSided)
    {
        _doubleSided = doubleSided;
        if (_doubleSided)
        {
            _stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
        }
        else
        {
            _stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
        }
    }

    bool MaterialAsset::removeParameter(const std::string& name)
    {
        if (_parameters.count(name))
        {
            MaterialParameterType& parameter = _parameters[name];
            if (parameter.index() == size_t(MaterialParameterTypeIndex::Texture))
            {
                TextureAndUnit& textureAndUnit = std::get<TextureAndUnit>(parameter);
                _stateSet->removeTextureAttribute(textureAndUnit.second, osg::StateAttribute::Type::TEXTURE);
            }
            _stateSet->removeUniform(_stateSet->getUniform(name));
            _parameters.erase(name);
            _uniformLines.erase(name);
            return true;
        }
        return false;
    }

    void MaterialAsset::setSource(const std::string& source)
    {
        _source = source;
        std::string shaderSource = "#version 460 core\n";
        for (const auto& uniformLine : _uniformLines)
            shaderSource += uniformLine.second;
        shaderSource += _source;
        _shader->setShaderSource(shaderSource);
    }

    void MaterialAsset::initializeShaderDefines()
    {
        _stateSet->setDefine("UNLIT", "0");
        _stateSet->setDefine("STANDARD", "1");
        _stateSet->setDefine("OPAQUE", "0");
        _stateSet->setDefine("ALPHA_MASK", "1");
        _stateSet->setDefine("ALPHA_BLEND", "2");
    }

    uint32_t MaterialAsset::getAvailableUnit()
    {
        std::unordered_set<uint32_t> unavailableUnit;
        for (const auto& parameter : _parameters)
            if (parameter.second.index() == size_t(MaterialParameterTypeIndex::Texture))
                unavailableUnit.insert(std::get<TextureAndUnit>(parameter.second).second);

        uint32_t availableUnit = 0;
        while (unavailableUnit.count(availableUnit))
            ++availableUnit;
        return availableUnit;
    }

    void dirty()
    {
        // update scene's all material instance
    }

}
