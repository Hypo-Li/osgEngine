#pragma once
#include "Texture.h"

#include <osg/Shader>
#include <variant>

template <typename T>
static constexpr bool is_shader_parameter_v =
    std::is_same_v<T, bool> ||
    std::is_same_v<T, int> ||
    std::is_same_v<T, float> ||
    std::is_same_v<T, osg::Vec2f> ||
    std::is_same_v<T, osg::Vec3f> ||
    std::is_same_v<T, osg::Vec4f> ||
    std::is_base_of_v<xxx::Texture, std::remove_pointer_t<T>>;

namespace xxx
{

    class Shader : public Object
    {
        REFLECT_CLASS(Shader)
    public:
        Shader();

        using Parameter = std::variant<bool, int, float, osg::Vec2f, osg::Vec3f, osg::Vec4f, osg::ref_ptr<Texture>>;

        void setSource(const std::string& source)
        {
            mSource = source;
        }

        const std::string& getSource() const
        {
            return mSource;
        }

        template <typename T, typename = std::enable_if_t<is_shader_parameter_v<T>>>
        void addParameter(const std::string& name, T defaultValue)
        {
            if constexpr (std::is_base_of_v<xxx::Texture, std::remove_pointer_t<T>>)
                mParameters.emplace(name, osg::ref_ptr<Texture>(defaultValue));
            else
                mParameters.emplace(name, defaultValue);
        }

        template <typename T, typename = std::enable_if_t<is_shader_parameter_v<T>>>
        void setParameter(const std::string& name, T value)
        {
            Shader::Parameter parameterValue;
            if constexpr (std::is_base_of_v<xxx::Texture, std::remove_pointer_t<T>>)
                parameterValue = osg::ref_ptr<Texture>(value);
            else
                parameterValue = value;

            auto findResult = mParameters.find(name);
            if (findResult != mParameters.end() && findResult->second.index() == parameterValue.index())
            {
                findResult->second = parameterValue;
            }
        }

        const auto& getParameters() const
        {
            return mParameters;
        }

    protected:
        std::map<std::string, Parameter> mParameters;
        std::string mSource;
    };

    namespace refl
    {
        template <>
        inline Type* Reflection::createType<osg::Vec2f>()
        {
            Struct* structure = new StructInstance<osg::Vec2f>("osg::Vec2f");
            structure->addProperty<0>("x", &osg::Vec2f::_v);
            structure->addProperty<1>("y", &osg::Vec2f::_v);
            return structure;
        }

        template <>
        inline Type* Reflection::createType<osg::Vec3f>()
        {
            Struct* structure = new StructInstance<osg::Vec3f>("osg::Vec3f");
            structure->addProperty<0>("x", &osg::Vec2f::_v);
            structure->addProperty<1>("y", &osg::Vec2f::_v);
            structure->addProperty<2>("z", &osg::Vec2f::_v);
            return structure;
        }

        template <>
        inline Type* Reflection::createType<osg::Vec4f>()
        {
            Struct* structure = new StructInstance<osg::Vec4f>("osg::Vec4f");
            structure->addProperty<0>("x", &osg::Vec2f::_v);
            structure->addProperty<1>("y", &osg::Vec2f::_v);
            structure->addProperty<2>("z", &osg::Vec2f::_v);
            structure->addProperty<3>("w", &osg::Vec2f::_v);
            return structure;
        }

        template <>
        inline Type* Reflection::createType<Shader>()
        {
            Class* clazz = new ClassInstance<Shader>("Shader");
            Property* propParameters = clazz->addProperty("Parameters", &Shader::mParameters);
            Property* propSource = clazz->addProperty("Source", &Shader::mSource);
            return clazz;
        }
    }
}
