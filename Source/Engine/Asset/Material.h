#pragma once
#include "AssetManager.h"
#include "Texture.h"

#include <variant>

namespace xxx
{
    class AMaterial : public Asset
    {
    public:
        AMaterial(Type type) : Asset(type) {}
        virtual ~AMaterial() = default;

        virtual osg::StateSet* getStateSet() const = 0;
    };

    class AMaterialTemplate : public AMaterial
    {
        friend class AMaterialInstance;
    public:
        AMaterialTemplate();
        virtual ~AMaterialTemplate() = default;

        virtual void serialize(Json& json, std::vector<char>& binary, std::vector<std::string>& reference) const override;

        virtual void deserialize(const Json& json, const std::vector<char>& binary, const std::vector<std::string>& reference) override;

        enum class ShadingModel
        {
            Unlit,
            Standard,
        };

        enum class AlphaMode
        {
            Opaque,
            Alpha_Mask,
            Alpha_Blend,
        };

        void setShadingModel(ShadingModel shadingModel);

        void setAlphaMode(AlphaMode alphaMode);

        void setDoubleSided(bool doubleSided);

        void setSource(const std::string& source);

        void apply();

        ShadingModel getShadingModel() const { return _shadingModel; }

        AlphaMode getAlphaMode() const { return _alphaMode; }

        bool getDoubleSided() const { return _doubleSided; }

        std::string& getSource() { return _source; }

        virtual osg::StateSet* getStateSet() const override { return _stateSet; }

        bool appendParameter(const std::string& name, bool value) { return appendParameter<bool>(name, value); }
        bool appendParameter(const std::string& name, int value) { return appendParameter<int>(name, value); }
        bool appendParameter(const std::string& name, float value) { return appendParameter<float>(name, value); }
        bool appendParameter(const std::string& name, osg::Vec2 value) { return appendParameter<osg::Vec2>(name, value); }
        bool appendParameter(const std::string& name, osg::Vec3 value) { return appendParameter<osg::Vec3>(name, value); }
        bool appendParameter(const std::string& name, osg::Vec4 value) { return appendParameter<osg::Vec4>(name, value); }
        bool appendParameter(const std::string& name, ATexture* value) { return appendParameter<ATexture*>(name, value); }

        void setParameter(const std::string& name, bool value) { setParameter<bool>(name, value); }
        void setParameter(const std::string& name, int value) { setParameter<int>(name, value); }
        void setParameter(const std::string& name, float value) { setParameter<float>(name, value); }
        void setParameter(const std::string& name, osg::Vec2 value) { setParameter<osg::Vec2>(name, value); }
        void setParameter(const std::string& name, osg::Vec3 value) { setParameter<osg::Vec3>(name, value); }
        void setParameter(const std::string& name, osg::Vec4 value) { setParameter<osg::Vec4>(name, value); }
        void setParameter(const std::string& name, ATexture* value) { setParameter<ATexture*>(name, value); }

        bool removeParameter(const std::string& name);

    private:
        osg::ref_ptr<osg::StateSet> _stateSet;
        osg::ref_ptr<osg::Shader> _shader;
        bool _stateSetDirty;
        bool _shaderDirty;
        ShadingModel _shadingModel;
        AlphaMode _alphaMode;
        bool _doubleSided;
        std::string _source;
        using TextureAndUnit = std::pair<osg::ref_ptr<ATexture>, int>;
        using Parameter = std::variant<bool, int, float, osg::Vec2, osg::Vec3, osg::Vec4, TextureAndUnit>;
        enum class ParameterIndex
        {
            Bool,
            Int,
            Float,
            Float2,
            Float3,
            Float4,
            Texture,
        };
        std::map<std::string, Parameter> _parameters;

        static constexpr unsigned int _sMaxTextureCount = 16;

        int getAvailableUnit();

        void applyParameters();

        static std::string getParameterTypeString(const Parameter& parameterType);

        template <typename T>
        bool appendParameter(const std::string& name, T value)
        {
            if (_parameters.count(name))
                return false;
            if constexpr (std::is_same_v<T, ATexture*>)
            {
                int unit = getAvailableUnit();
                if (unit >= _sMaxTextureCount)
                    return false;
                _parameters[name] = std::make_pair(value, unit);
            }
            else
            {
                _parameters[name] = value;
            }
            _stateSetDirty = true;
            _shaderDirty = true;
            return true;
        }

        template <typename T>
        void setParameter(const std::string& name, T value)
        {
            auto& findResult = _parameters.find(name);
            if (findResult != _parameters.end())
            {
                if constexpr (std::is_same_v<T, ATexture*>)
                {
                    TextureAndUnit& textureAndUnit = std::get<TextureAndUnit>(findResult->second);
                    textureAndUnit.first = value;
                    int unit = textureAndUnit.second;
                    //_stateSet->setTextureAttribute(unit, value->_texture, osg::StateAttribute::ON);
                    _stateSetDirty = true;
                }
                else
                {
                    bool typeIsSame = false;
                    ParameterIndex parameterIndex = static_cast<ParameterIndex>(findResult->second.index());
                    if constexpr (std::is_same_v<T, bool>)
                        typeIsSame = parameterIndex == ParameterIndex::Bool;
                    else if constexpr (std::is_same_v<T, int>)
                        typeIsSame = parameterIndex == ParameterIndex::Int;
                    else if constexpr (std::is_same_v<T, float>)
                        typeIsSame = parameterIndex == ParameterIndex::Float;
                    else if constexpr (std::is_same_v<T, osg::Vec2>)
                        typeIsSame = parameterIndex == ParameterIndex::Float2;
                    else if constexpr (std::is_same_v<T, osg::Vec3>)
                        typeIsSame = parameterIndex == ParameterIndex::Float3;
                    else if constexpr (std::is_same_v<T, osg::Vec4>)
                        typeIsSame = parameterIndex == ParameterIndex::Float4;

                    if (!typeIsSame)
                    {
                        // Warning: wrong parameter type
                        return;
                    }

                    findResult->second = value;
                    //_stateSet->getUniform("u" + name)->set(value);
                    _stateSetDirty = true;
                }
            }
        }
    };

    class AMaterialInstance : public AMaterial
    {
        friend class CMeshRenderer;
    public:
        AMaterialInstance();
        virtual ~AMaterialInstance() = default;

        virtual void serialize(Json& json, std::vector<char>& binary, std::vector<std::string>& reference) const override;

        virtual void deserialize(const Json& json, const std::vector<char>& binary, const std::vector<std::string>& reference) override;

        void setMaterialTemplate(AMaterialTemplate* materialTemplate)
        {
            if (materialTemplate == _materialTemplate)
                return;
            _materialTemplate = materialTemplate;
            _stateSet = new osg::StateSet(*_materialTemplate->getStateSet(), osg::CopyOp::DEEP_COPY_UNIFORMS);
            _parameters.clear();
            for (const auto& parameter : _materialTemplate->_parameters)
                _parameters[parameter.first] = { false, parameter.second, parameter.second };
        }

        void syncMaterialTemplate()
        {
            osg::StateSet* newStateSet = new osg::StateSet(*_materialTemplate->getStateSet(), osg::CopyOp::DEEP_COPY_UNIFORMS);
            auto itr = _parameters.begin();
            while (itr != _parameters.end())
            {
                auto findResult = _materialTemplate->_parameters.find(itr->first);
                if (findResult == _materialTemplate->_parameters.end())
                {
                    // case 1: 参数在新材质模板中不存在
                    _parameters.erase(itr++);
                }
                else if (itr->second.defaultValue.index() != findResult->second.index())
                {
                    // case 2: 参数在新材质中存在, 但类型不同
                    itr->second.enable = false;
                    itr->second.defaultValue = findResult->second;
                    itr->second.overrideValue = findResult->second;
                    itr++;
                }
                else
                {
                    // case 3: 参数在新材质中存在, 且类型相同
                    std::string uniformName = "u" + itr->first;
                    newStateSet->getUniformList().at(uniformName).first = _stateSet->getUniform(uniformName);
                    itr++;
                }
            }
            // 添加新参数
            for (auto& parameter : _materialTemplate->_parameters)
            {
                if (!_parameters.count(parameter.first))
                    _parameters[parameter.first] = { false, parameter.second, parameter.second };
            }
            _stateSet = newStateSet;
        }

        AMaterialTemplate* getMaterialTemplate() { return _materialTemplate; }

        virtual osg::StateSet* getStateSet() const override { return _stateSet; }

        void setEnableParameter(const std::string& name, bool enable)
        {
            auto& findResult = _parameters.find(name);
            if (findResult != _parameters.end())
            {
                findResult->second.enable = enable;
                Parameter& parameter = enable ? findResult->second.overrideValue : findResult->second.defaultValue;
                osg::Uniform* uniform = _stateSet->getUniform("u" + name);
                switch (parameter.index())
                {
                case static_cast<size_t>(ParameterIndex::Bool):
                    uniform->set(std::get<bool>(parameter)); break;
                case static_cast<size_t>(ParameterIndex::Int):
                    uniform->set(std::get<int>(parameter)); break;
                case static_cast<size_t>(ParameterIndex::Float):
                    uniform->set(std::get<float>(parameter)); break;
                case static_cast<size_t>(ParameterIndex::Float2):
                    uniform->set(std::get<osg::Vec2>(parameter)); break;
                case static_cast<size_t>(ParameterIndex::Float3):
                    uniform->set(std::get<osg::Vec3>(parameter)); break;
                case static_cast<size_t>(ParameterIndex::Float4):
                    uniform->set(std::get<osg::Vec4>(parameter)); break;
                case static_cast<size_t>(ParameterIndex::Texture):
                {
                    TextureAndUnit& textureAndUnit = std::get<TextureAndUnit>(parameter);
                    _stateSet->setTextureAttribute(textureAndUnit.second, textureAndUnit.first->_texture, osg::StateAttribute::ON);
                    break;
                }
                default:
                    break;
                }
            }
        }

        void setParameter(const std::string& name, bool value) { setParameter<bool>(name, value); }
        void setParameter(const std::string& name, int value) { setParameter<int>(name, value); }
        void setParameter(const std::string& name, float value) { setParameter<float>(name, value); }
        void setParameter(const std::string& name, osg::Vec2 value) { setParameter<osg::Vec2>(name, value); }
        void setParameter(const std::string& name, osg::Vec3 value) { setParameter<osg::Vec3>(name, value); }
        void setParameter(const std::string& name, osg::Vec4 value) { setParameter<osg::Vec4>(name, value); }
        void setParameter(const std::string& name, ATexture* value) { setParameter<ATexture*>(name, value); }

    private:
        using TextureAndUnit = AMaterialTemplate::TextureAndUnit;
        using Parameter = AMaterialTemplate::Parameter;
        using ParameterIndex = AMaterialTemplate::ParameterIndex;
        osg::ref_ptr<AMaterialTemplate> _materialTemplate;
        osg::ref_ptr<osg::StateSet> _stateSet;
        struct InstanceParameter
        {
            bool enable;
            Parameter defaultValue;
            Parameter overrideValue;
        };
        std::map<std::string, InstanceParameter> _parameters;

        template <typename T>
        void setParameter(const std::string& name, T value)
        {
            auto& findResult = _parameters.find(name);
            if (findResult != _parameters.end())
            {
                if constexpr (std::is_same_v<T, ATexture*>)
                {
                    findResult->second.enable = true;
                    findResult->second.overrideValue = value;
                    int unit = std::get<TextureAndUnit>(findResult->second.defaultValue).second;
                    _stateSet->setTextureAttribute(unit, value->_texture, osg::StateAttribute::ON);
                }
                else
                {
                    bool typeIsSame = false;
                    ParameterIndex parameterIndex = static_cast<ParameterIndex>(findResult->second.defaultValue.index());
                    if constexpr (std::is_same_v<T, bool>)
                        typeIsSame = parameterIndex == ParameterIndex::Bool;
                    else if constexpr (std::is_same_v<T, int>)
                        typeIsSame = parameterIndex == ParameterIndex::Int;
                    else if constexpr (std::is_same_v<T, float>)
                        typeIsSame = parameterIndex == ParameterIndex::Float;
                    else if constexpr (std::is_same_v<T, osg::Vec2>)
                        typeIsSame = parameterIndex == ParameterIndex::Float2;
                    else if constexpr (std::is_same_v<T, osg::Vec3>)
                        typeIsSame = parameterIndex == ParameterIndex::Float3;
                    else if constexpr (std::is_same_v<T, osg::Vec4>)
                        typeIsSame = parameterIndex == ParameterIndex::Float4;

                    if (!typeIsSame)
                    {
                        // Warning: wrong parameter type
                        return;
                    }

                    findResult->second.enable = true;
                    findResult->second.overrideValue = value;
                    _stateSet->getUniform("u" + name)->set(value);
                }
            }
        }
    };
}
