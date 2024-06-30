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
        MaterialAsset();
        virtual ~MaterialAsset() = default;

        virtual void serialize(Json& json, std::vector<char>& binray, std::vector<std::string>& reference) const override;

        virtual void deserialize(const Json& json, const std::vector<char>& binray, const std::vector<std::string>& reference) override;

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

        ShadingModel getShadingModel() const { return _shadingModel; }

        void setAlphaBlend(AlphaMode alphaMode);

        AlphaMode getAlphaMode() const { return _alphaMode; }

        void setDoubleSided(bool doubleSided);

        bool getDoubleSided() const { return _doubleSided; }

        bool appendParameter(const std::string&& name, bool value)
        {
            return appendParameter<bool>(name, value);
        }

        void setParameter(const std::string&& name, bool value)
        {
            setParameter<bool>(name, value);
        }

        bool appendParameter(const std::string& name, int value)
        {
            return appendParameter<int>(name, value);
        }

        void setParameter(const std::string&& name, int value)
        {
            setParameter<int>(name, value);
        }

        bool appendParameter(const std::string& name, float value)
        {
            return appendParameter<float>(name, value);
        }

        void setParameter(const std::string&& name, float value)
        {
            setParameter<float>(name, value);
        }

        bool appendParameter(const std::string& name, osg::Vec2 value)
        {
            return appendParameter<osg::Vec2>(name, value);
        }

        void setParameter(const std::string&& name, osg::Vec2 value)
        {
            setParameter<osg::Vec2>(name, value);
        }

        bool appendParameter(const std::string& name, osg::Vec3 value)
        {
            return appendParameter<osg::Vec3>(name, value);
        }

        void setParameter(const std::string&& name, osg::Vec3 value)
        {
            setParameter<osg::Vec3>(name, value);
        }

        bool appendParameter(const std::string& name, osg::Vec4 value)
        {
            return appendParameter<osg::Vec4>(name, value);
        }

        void setParameter(const std::string&& name, osg::Vec4 value)
        {
            setParameter<osg::Vec4>(name, value);
        }

        bool appendParameter(const std::string& name, TextureAsset* value)
        {
            return appendParameter<TextureAsset*>(name, value);
        }

        void setParameter(const std::string&& name, TextureAsset* value)
        {
            setParameter<TextureAsset*>(name, value);
        }

        bool removeParameter(const std::string& name);

        void setSource(const std::string& source);

    private:
        osg::ref_ptr<osg::StateSet> _stateSet;
        osg::ref_ptr<osg::Shader> _shader;
        std::string _source;
        ShadingModel _shadingModel;
        AlphaMode _alphaMode;
        bool _doubleSided;
        using TextureAndUnit = std::pair<osg::ref_ptr<TextureAsset>, uint32_t>;
        using MaterialParameterType = std::variant<bool, int, float, osg::Vec2, osg::Vec3, osg::Vec4, TextureAndUnit>;
        enum class MaterialParameterTypeIndex
        {
            Bool,
            Int,
            Float,
            Float2,
            Float3,
            Float4,
            Texture,
        };
        std::map<std::string, MaterialParameterType> _parameters;
        std::map<std::string, std::string> _uniformLines;

        static constexpr unsigned int _sMaxTextureCount = 16;
        
        void initializeShaderDefines();

        uint32_t getAvailableUnit();

        static Json osgVec2ToJson(const osg::Vec2& v)
        {
            return Json::array({ v.x(), v.y() });
        }

        static Json osgVec3ToJson(const osg::Vec3& v)
        {
            return Json::array({ v.x(), v.y(), v.z() });
        }

        static Json osgVec4ToJson(const osg::Vec4& v)
        {
            return Json::array({ v.x(), v.y(), v.z() });
        }

        static osg::Vec2 jsonToOsgVec2(const Json& json)
        {
            return osg::Vec2(json[0].get<float>(), json[1].get<float>());
        }

        static osg::Vec3 jsonToOsgVec3(const Json& json)
        {
            return osg::Vec3(json[0].get<float>(), json[1].get<float>(), json[2].get<float>());
        }

        static osg::Vec4 jsonToOsgVec4(const Json& json)
        {
            return osg::Vec4(json[0].get<float>(), json[1].get<float>(), json[2].get<float>(), json[3].get<float>());
        }

        template <typename T>
        static std::string getTypeNameString()
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

        static std::string getTextureTypeNameString(TextureAsset* textureAsset)
        {
            switch (textureAsset->_texture->getTextureTarget())
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

        template <typename T>
        bool appendParameter(const std::string& name, T value)
        {
            if (_parameters.count(name))
                return false;
            if constexpr (std::is_same_v<T, TextureAsset*>)
            {
                uint32_t unit = getAvailableUnit();
                if (unit >= _sMaxTextureCount)
                    return false;
                _parameters[name] = std::make_pair(value, unit);
                _stateSet->addUniform(new osg::Uniform(name.c_str(), unit));
                _stateSet->setTextureAttribute(unit, value->_texture, osg::StateAttribute::ON);
                _uniformLines[name] = "uniform " + getTextureTypeNameString(value) + " u" + name + ";\n";
            }
            else
            {
                _parameters[name] = value;
                _stateSet->addUniform(new osg::Uniform(name.c_str(), value));
                _uniformLines[name] = "uniform " + getTypeNameString<T>() + " u" + name + ";\n";
            }
            // uniforms changed, need refresh
            dirty();
            return true;
        }

        template <typename T>
        void setParameter(const std::string& name, T value)
        {
            if (_parameters.count(name))
            {
                if constexpr (std::is_same_v<T, TextureAsset*>)
                {
                    TextureAndUnit& textureAndUnit = std::get<TextureAndUnit>(_parameters[name]);
                    textureAndUnit.first = value;
                    uint32_t unit = textureAndUnit.second;
                    _stateSet->setTextureAttribute(unit, value->_texture, osg::StateAttribute::ON);
                }
                else
                {
                    _parameters[name] = value;
                    _stateSet->getUniform(name)->set(value);
                }
            }
        }
    };
}
