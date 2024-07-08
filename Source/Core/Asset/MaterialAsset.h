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
        std::string _source;
        ShadingModel _shadingModel;
        AlphaMode _alphaMode;
        bool _doubleSided;
        using TextureAssetAndUnit = std::pair<osg::ref_ptr<TextureAsset>, int>;
        using ParameterType = std::variant<bool, int, float, osg::Vec2, osg::Vec3, osg::Vec4, TextureAssetAndUnit>;
        enum class ParameterTypeIndex
        {
            Bool,
            Int,
            Float,
            Float2,
            Float3,
            Float4,
            Texture,
        };
        std::map<std::string, ParameterType> _parameters;
        std::map<std::string, std::string> _uniformLines;

        static constexpr unsigned int _sMaxTextureCount = 16;
        static const std::unordered_map<GLenum, std::string> _sTextureSamplerStringMap;

        int getAvailableUnit();

        template <typename T>
        static std::string getParameterTypeString()
        {
            if constexpr (std::is_same_v<T, bool>)
                return "bool";
            else if constexpr (std::is_same_v<T, int>)
                return "int";
            else if constexpr (std::is_same_v<T, float>)
                return "float";
            else if constexpr (std::is_same_v<T, osg::Vec2>)
                return "vec2";
            else if constexpr (std::is_same_v<T, osg::Vec3>)
                return "vec3";
            else if constexpr (std::is_same_v<T, osg::Vec4>)
                return "vec4";
            else
                return "";
        }

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
                _stateSet->addUniform(new osg::Uniform(("u" + name).c_str(), unit));
                _stateSet->setTextureAttribute(unit, value->_texture, osg::StateAttribute::ON);
                _uniformLines[name] = "uniform " + _sTextureSamplerStringMap.at(value->_texture->getTextureTarget()) + " u" + name + ";\n";
            }
            else
            {
                _parameters[name] = value;
                _stateSet->addUniform(new osg::Uniform(("u" + name).c_str(), value));
                _uniformLines[name] = "uniform " + getParameterTypeString<T>() + " u" + name + ";\n";
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
                    TextureAssetAndUnit textureAssetAndUnit = std::get<TextureAssetAndUnit>(findResult->second);
                    textureAssetAndUnit.first = value;
                    int unit = textureAssetAndUnit.second;
                    _stateSet->setTextureAttribute(unit, value->_texture, osg::StateAttribute::ON);
                }
                else
                {
                    bool typeIsSame = false;
                    ParameterTypeIndex currentTypeIndex = static_cast<ParameterTypeIndex>(findResult->second.index());
                    if constexpr (std::is_same_v<T, bool>)
                        typeIsSame = currentTypeIndex == ParameterTypeIndex::Bool;
                    else if constexpr (std::is_same_v<T, int>)
                        typeIsSame = currentTypeIndex == ParameterTypeIndex::Int;
                    else if constexpr (std::is_same_v<T, float>)
                        typeIsSame = currentTypeIndex == ParameterTypeIndex::Float;
                    else if constexpr (std::is_same_v<T, osg::Vec2>)
                        typeIsSame = currentTypeIndex == ParameterTypeIndex::Float2;
                    else if constexpr (std::is_same_v<T, osg::Vec3>)
                        typeIsSame = currentTypeIndex == ParameterTypeIndex::Float3;
                    else if constexpr (std::is_same_v<T, osg::Vec4>)
                        typeIsSame = currentTypeIndex == ParameterTypeIndex::Float4;

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
        osg::ref_ptr<MaterialTemplateAsset> _materialTemplate;
        osg::ref_ptr<osg::StateSet> _stateSet;
        using TextureAssetAndUnit = MaterialTemplateAsset::TextureAssetAndUnit;
        using ParameterType = MaterialTemplateAsset::ParameterType;
        using ParameterTypeIndex = MaterialTemplateAsset::ParameterTypeIndex;
        std::map<std::string, ParameterType> _parameters;

        template <typename T>
        void setParameter(const std::string& name, T value)
        {
            auto& findResult = _materialTemplate->_parameters.find(name);
            if (findResult != _materialTemplate->_parameters.end())
            {
                if constexpr (std::is_same_v<T, TextureAsset*>)
                {
                    TextureAssetAndUnit textureAssetAndUnit = std::get<TextureAssetAndUnit>(findResult->second);
                    int unit = textureAssetAndUnit.second;
                    _parameters[name] = std::make_pair(value, unit);
                    _stateSet->setTextureAttribute(unit, value->_texture, osg::StateAttribute::ON);
                }
                else
                {
                    bool typeIsSame = false;
                    ParameterTypeIndex currentTypeIndex = static_cast<ParameterTypeIndex>(findResult->second.index());
                    if constexpr (std::is_same_v<T, bool>)
                        typeIsSame = currentTypeIndex == ParameterTypeIndex::Bool;
                    else if constexpr (std::is_same_v<T, int>)
                        typeIsSame = currentTypeIndex == ParameterTypeIndex::Int;
                    else if constexpr (std::is_same_v<T, float>)
                        typeIsSame = currentTypeIndex == ParameterTypeIndex::Float;
                    else if constexpr (std::is_same_v<T, osg::Vec2>)
                        typeIsSame = currentTypeIndex == ParameterTypeIndex::Float2;
                    else if constexpr (std::is_same_v<T, osg::Vec3>)
                        typeIsSame = currentTypeIndex == ParameterTypeIndex::Float3;
                    else if constexpr (std::is_same_v<T, osg::Vec4>)
                        typeIsSame = currentTypeIndex == ParameterTypeIndex::Float4;

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
