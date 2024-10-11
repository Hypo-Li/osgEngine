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

template<typename T, typename V, size_t I = 0>
constexpr size_t variant_index()
{
    if constexpr (I >= std::variant_size_v<V>)
    {
        return (std::variant_size_v<V>);
    }
    else
    {
        if constexpr (std::is_same_v<std::variant_alternative_t<I, V>, T>)
        {
            return (I);
        }
        else
        {
            return (variant_index<T, V, I + 1>());
        }
    }
}

namespace xxx
{

    class Shader : public Object
    {
        REFLECT_CLASS(Shader)
    public:
        Shader();

        using TextureAndUnit = std::pair<osg::ref_ptr<Texture>, int>;
        using ParameterValue = std::variant<bool, int, float, osg::Vec2f, osg::Vec3f, osg::Vec4f, TextureAndUnit>;
        enum class ParameterIndex
        {
            Bool,
            Int,
            Float,
            Vec2f,
            Vec3f,
            Vec4f,
            Texture,
        };
        using Parameters = std::map<std::string, ParameterValue>;

        void setSource(const std::string& source)
        {
            mSource = source;
        }

        std::string& getSource()
        {
            return mSource;
        }

        const std::string& getSource() const
        {
            return mSource;
        }

        template <typename T, typename = std::enable_if_t<is_shader_parameter_v<T>>>
        void addParameter(const std::string& name, T defaultValue)
        {
            if constexpr (std::is_base_of_v<xxx::Texture, std::remove_pointer_t<T>>)
                mParameters.emplace(name, std::make_pair(osg::ref_ptr<Texture>(defaultValue), getAvailableTextureUnit()));
            else
                mParameters.emplace(name, defaultValue);
        }

        template <typename T, typename = std::enable_if_t<is_shader_parameter_v<T>>>
        void setParameter(const std::string& name, T value)
        {
            auto findResult = mParameters.find(name);
            if constexpr (std::is_base_of_v<xxx::Texture, std::remove_pointer_t<T>>)
            {
                if (findResult != mParameters.end() && findResult->second.index() == size_t(ParameterIndex::Texture))
                    findResult->second = std::make_pair(osg::ref_ptr<Texture>(value), std::get<TextureAndUnit>(findResult->second).second);
            }
            else
            {
                if (findResult != mParameters.end() && findResult->second.index() == variant_index<T, ParameterValue>())
                    findResult->second = value;
            }
        }

        const Parameters& getParameters() const
        {
            return mParameters;
        }

    protected:
        Parameters mParameters;
        std::string mSource;

        int getAvailableTextureUnit()
        {
            std::unordered_set<int> unavailableTextureUnit;
            for (const auto& param : mParameters)
            {
                if (param.second.index() == size_t(ParameterIndex::Texture))
                {
                    int unit = std::get<TextureAndUnit>(param.second).second;
                    unavailableTextureUnit.insert(unit);
                }
            }

            int availableTextureUnit = 0;
            while (unavailableTextureUnit.count(availableTextureUnit))
                availableTextureUnit++;

            const int MaxTextureCount = 16;
            if (availableTextureUnit >= MaxTextureCount)
            {
                // LogError
                ;
            }

            return availableTextureUnit;
        }
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
