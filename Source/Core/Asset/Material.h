#pragma once
#include "Asset.h"
#include "Texture.h"
#include <osg/StateSet>

namespace xxx
{
    class Material : public Asset
    {
    public:
        Material(const fs::path& assetPath) : Asset(assetPath) {}
        virtual ~Material() = default;

        virtual bool load(const fs::path& assetPath);
        virtual bool save(const fs::path& assetPath) const;

        enum class ShadingModel
        {
            Unlit,
            Standard,
        };

        enum class AlphaMode
        {
            Opaque,
            Alpha_Blend,
            Alpha_Mask,
        };

        void setShadingModel(ShadingModel shadingModel) { _shadingModel = shadingModel; }
        ShadingModel getShadingModel() const { return _shadingModel; }
        void setAlphaBlend(AlphaMode alphaMode) { _alphaMode = alphaMode; }
        AlphaMode getAlphaMode() const { return _alphaMode; }
        void setDoubleSided(bool doubleSided) { _doubleSided = doubleSided; }
        bool getDoubleSided() const { return _doubleSided; }

        bool appendParameter(std::string_view name, bool value)
        {
            if (_parameters.count(name))
                return false;
            auto& parameter = _parameters[name];
            parameter.type = Parameter::Type::Bool;
            parameter.sampleTypeValue.boolValue = value;
            return true;
        }
        bool appendParameter(std::string_view name, int value)
        {
            if (_parameters.count(name))
                return false;
            auto& parameter = _parameters[name];
            parameter.type = Parameter::Type::Int;
            parameter.sampleTypeValue.intValue = value;
            return true;
        }
        bool appendParameter(std::string_view name, unsigned int value)
        {
            if (_parameters.count(name))
                return false;
            auto& parameter = _parameters[name];
            parameter.type = Parameter::Type::Uint;
            parameter.sampleTypeValue.uintValue = value;
            return true;
        }
        bool appendParameter(std::string_view name, float value)
        {
            if (_parameters.count(name))
                return false;
            auto& parameter = _parameters[name];
            parameter.type = Parameter::Type::Float;
            parameter.sampleTypeValue.floatValue = value;
            return true;
        }
        bool appendParameter(std::string_view name, osg::Vec2 value)
        {
            if (_parameters.count(name))
                return false;
            auto& parameter = _parameters[name];
            parameter.type = Parameter::Type::Float2;
            parameter.sampleTypeValue.float2Value = value;
            return true;
        }
        bool appendParameter(std::string_view name, osg::Vec3 value)
        {
            if (_parameters.count(name))
                return false;
            auto& parameter = _parameters[name];
            parameter.type = Parameter::Type::Float3;
            parameter.sampleTypeValue.float3Value = value;
            return true;
        }
        bool appendParameter(std::string_view name, osg::Vec4 value)
        {
            if (_parameters.count(name))
                return false;
            auto& parameter = _parameters[name];
            parameter.type = Parameter::Type::Float4;
            parameter.sampleTypeValue.float4Value = value;
            return true;
        }
        bool appendParameter(std::string_view name, Texture* value)
        {
            if (_parameters.count(name))
                return false;
            auto& parameter = _parameters[name];
            parameter.type = Parameter::Type::Texture;
            parameter.textureValue = value;
            return true;
        }
        bool removeParameter(std::string_view name)
        {
            if (!_parameters.count(name))
                return false;
            _parameters.erase(name);
            return true;
        }

    private:
        osg::ref_ptr<osg::StateSet> _material;
        ShadingModel _shadingModel;
        AlphaMode _alphaMode;
        bool _doubleSided;

        struct Parameter
        {
            enum class Type
            {
                Bool,
                Int,
                Uint,
                Float,
                Float2,
                Float3,
                Float4,
                Texture,
            } type;
            union SampleType
            {
                bool boolValue;
                int intValue;
                unsigned int uintValue;
                float floatValue;
                osg::Vec2 float2Value;
                osg::Vec3 float3Value;
                osg::Vec4 float4Value;
                SampleType() {}
            } sampleTypeValue;
            osg::ref_ptr<Texture> textureValue;
        };
        std::map<std::string_view, Parameter> _parameters;

        static const std::string_view _sUnlitMaterialDefaultShaderSource;
        static const std::string_view _sStandardMaterialDefaultShaderSource;
    };
}
