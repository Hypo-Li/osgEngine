#pragma once
#include "AssetManager.h"
#include "TextureAsset.h"
#include <osg/StateSet>
#include <variant>
#include <unordered_set>

namespace xxx
{
    class MeshRenderer;
    class MaterialAsset : public Asset
    {
        friend class MeshRenderer;
    public:
        MaterialAsset(Type type) : Asset(type) {}
        virtual ~MaterialAsset() = default;

        virtual osg::StateSet* getStateSet() const = 0;
        virtual osg::Shader* getShader() const = 0;
    };

    class ImGuiHandler;
    class MaterialInstanceAsset;
    class MaterialTemplateAsset : public MaterialAsset
    {
        friend class ImGuiHandler;
        friend class MeshRenderer;
        friend class MaterialInstanceAsset;
    public:
        MaterialTemplateAsset();
        virtual ~MaterialTemplateAsset() = default;

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

        virtual osg::Shader* getShader() const override { return _shader; }

        bool appendParameter(const std::string& name, bool value) { return appendParameter<bool>(name, value); }
        bool appendParameter(const std::string& name, int value) { return appendParameter<int>(name, value); }
        bool appendParameter(const std::string& name, float value) { return appendParameter<float>(name, value); }
        bool appendParameter(const std::string& name, osg::Vec2 value) { return appendParameter<osg::Vec2>(name, value); }
        bool appendParameter(const std::string& name, osg::Vec3 value) { return appendParameter<osg::Vec3>(name, value); }
        bool appendParameter(const std::string& name, osg::Vec4 value) { return appendParameter<osg::Vec4>(name, value); }
        bool appendParameter(const std::string& name, TextureAsset* value) { return appendParameter<TextureAsset*>(name, value); }

        void setParameter(const std::string& name, bool value) { setParameter<bool>(name, value); }
        void setParameter(const std::string& name, int value) { setParameter<int>(name, value); }
        void setParameter(const std::string& name, float value) { setParameter<float>(name, value); }
        void setParameter(const std::string& name, osg::Vec2 value) { setParameter<osg::Vec2>(name, value); }
        void setParameter(const std::string& name, osg::Vec3 value) { setParameter<osg::Vec3>(name, value); }
        void setParameter(const std::string& name, osg::Vec4 value) { setParameter<osg::Vec4>(name, value); }
        void setParameter(const std::string& name, TextureAsset* value) { setParameter<TextureAsset*>(name, value); }

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
        using TextureAssetAndUnit = std::pair<osg::ref_ptr<TextureAsset>, int>;
        using Parameter = std::variant<bool, int, float, osg::Vec2, osg::Vec3, osg::Vec4, TextureAssetAndUnit>;
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
            if constexpr (std::is_same_v<T, TextureAsset*>)
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
                if constexpr (std::is_same_v<T, TextureAsset*>)
                {
                    TextureAssetAndUnit& textureAssetAndUnit = std::get<TextureAssetAndUnit>(findResult->second);
                    textureAssetAndUnit.first = value;
                    int unit = textureAssetAndUnit.second;
                    //_stateSet->setTextureAttribute(unit, value->_texture, osg::StateAttribute::ON);
                    _stateSetDirty = true;
                }
                else
                {
                    bool typeIsSame = false;
                    ParameterIndex currentTypeIndex = static_cast<ParameterIndex>(findResult->second.index());
                    if constexpr (std::is_same_v<T, bool>)
                        typeIsSame = currentTypeIndex == ParameterIndex::Bool;
                    else if constexpr (std::is_same_v<T, int>)
                        typeIsSame = currentTypeIndex == ParameterIndex::Int;
                    else if constexpr (std::is_same_v<T, float>)
                        typeIsSame = currentTypeIndex == ParameterIndex::Float;
                    else if constexpr (std::is_same_v<T, osg::Vec2>)
                        typeIsSame = currentTypeIndex == ParameterIndex::Float2;
                    else if constexpr (std::is_same_v<T, osg::Vec3>)
                        typeIsSame = currentTypeIndex == ParameterIndex::Float3;
                    else if constexpr (std::is_same_v<T, osg::Vec4>)
                        typeIsSame = currentTypeIndex == ParameterIndex::Float4;

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

    class MaterialInstanceAsset : public MaterialAsset
    {
    public:
        MaterialInstanceAsset();
        virtual ~MaterialInstanceAsset() = default;

        virtual void serialize(Json& json, std::vector<char>& binary, std::vector<std::string>& reference) const override;

        virtual void deserialize(const Json& json, const std::vector<char>& binary, const std::vector<std::string>& reference) override;

        void setMaterialTemplate(MaterialTemplateAsset* materialTemplate);

        void syncMaterialTemplate();

        MaterialTemplateAsset* getMaterialTemplate() { return _materialTemplate; }

        virtual osg::StateSet* getStateSet() const override { return _stateSet; }

        virtual osg::Shader* getShader() const override { return _materialTemplate->getShader(); }

        void setParameter(const std::string& name, bool value) { setParameter<bool>(name, value); }
        void setParameter(const std::string& name, int value) { setParameter<int>(name, value); }
        void setParameter(const std::string& name, float value) { setParameter<float>(name, value); }
        void setParameter(const std::string& name, osg::Vec2 value) { setParameter<osg::Vec2>(name, value); }
        void setParameter(const std::string& name, osg::Vec3 value) { setParameter<osg::Vec3>(name, value); }
        void setParameter(const std::string& name, osg::Vec4 value) { setParameter<osg::Vec4>(name, value); }
        void setParameter(const std::string& name, TextureAsset* value) { setParameter<TextureAsset*>(name, value); }

    private:
        using TextureAssetAndUnit = MaterialTemplateAsset::TextureAssetAndUnit;
        using Parameter = MaterialTemplateAsset::Parameter;
        using ParameterIndex = MaterialTemplateAsset::ParameterIndex;

        osg::ref_ptr<MaterialTemplateAsset> _materialTemplate;
        osg::ref_ptr<osg::StateSet> _stateSet;
        std::map<std::string, Parameter> _parameters;

        template <typename T>
        void setParameter(const std::string& name, T value)
        {
            auto& findResult = _materialTemplate->_parameters.find(name);
            if (findResult != _materialTemplate->_parameters.end())
            {
                if constexpr (std::is_same_v<T, TextureAsset*>)
                {
                    TextureAssetAndUnit& textureAssetAndUnit = std::get<TextureAssetAndUnit>(findResult->second);
                    int unit = textureAssetAndUnit.second;
                    _parameters[name] = std::make_pair(value, unit);
                    _stateSet->setTextureAttribute(unit, value->_texture, osg::StateAttribute::ON);
                }
                else
                {
                    bool typeIsSame = false;
                    ParameterIndex currentTypeIndex = static_cast<ParameterIndex>(findResult->second.index());
                    if constexpr (std::is_same_v<T, bool>)
                        typeIsSame = currentTypeIndex == ParameterIndex::Bool;
                    else if constexpr (std::is_same_v<T, int>)
                        typeIsSame = currentTypeIndex == ParameterIndex::Int;
                    else if constexpr (std::is_same_v<T, float>)
                        typeIsSame = currentTypeIndex == ParameterIndex::Float;
                    else if constexpr (std::is_same_v<T, osg::Vec2>)
                        typeIsSame = currentTypeIndex == ParameterIndex::Float2;
                    else if constexpr (std::is_same_v<T, osg::Vec3>)
                        typeIsSame = currentTypeIndex == ParameterIndex::Float3;
                    else if constexpr (std::is_same_v<T, osg::Vec4>)
                        typeIsSame = currentTypeIndex == ParameterIndex::Float4;

                    if (!typeIsSame)
                    {
                        // Warning: wrong parameter type
                        return;
                    }

                    _parameters[name] = value;
                    _stateSet->getUniform("u" + name)->set(value);
                }
            }
        }
    };
}
