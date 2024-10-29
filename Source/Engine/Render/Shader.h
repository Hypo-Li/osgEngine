#pragma once
#include "Texture.h"
#include "Texture2D.h"
#include "Texture2DArray.h"
#include "Texture3D.h"
#include "TextureCubemap.h"
#include <Engine/Core/AssetManager.h>

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
    std::is_same_v<std::remove_pointer_t<T>, xxx::Texture2D> ||
    std::is_same_v<std::remove_pointer_t<T>, xxx::Texture2DArray> ||
    std::is_same_v<std::remove_pointer_t<T>, xxx::Texture3D> ||
    std::is_same_v<std::remove_pointer_t<T>, xxx::TextureCubemap>;

template<typename T, typename V, size_t I = 0>
constexpr size_t variant_index()
{
    if constexpr (I >= std::variant_size_v<V>)
        return (std::variant_size_v<V>);
    else
    {
        if constexpr (std::is_same_v<std::variant_alternative_t<I, V>, T>)
            return (I);
        else
            return (variant_index<T, V, I + 1>());
    }
}

namespace xxx
{
    class Shader : public Object
    {
        REFLECT_CLASS(Shader)
    public:
        Shader();

        enum class ParameterType
        {
            Bool,
            Int,
            Float,
            Vec2f,
            Vec3f,
            Vec4f,
            Texture2D,
            Texture2DArray,
            Texture3D,
            TextureCubemap,
        };
        using Texture2DUnitPair = std::pair<osg::ref_ptr<Texture2D>, int>;
        using Texture2DArrayUnitPair = std::pair<osg::ref_ptr<Texture2DArray>, int>;
        using Texture3DUnitPair = std::pair<osg::ref_ptr<Texture3D>, int>;
        using TextureCubemapUnitPair = std::pair<osg::ref_ptr<TextureCubemap>, int>;
        using ParameterValue = std::variant<
            bool,
            int,
            float,
            osg::Vec2f,
            osg::Vec3f,
            osg::Vec4f,
            Texture2DUnitPair,
            Texture2DArrayUnitPair,
            Texture3DUnitPair,
            TextureCubemapUnitPair
        >;
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
                mParameters.emplace(name, std::make_pair(osg::ref_ptr<std::remove_pointer_t<T>>(defaultValue), assignTextureUnit()));
            else
                mParameters.emplace(name, defaultValue);
        }

        template <typename T, typename = std::enable_if_t<is_shader_parameter_v<T>>>
        void setParameter(const std::string& name, T value)
        {
            auto findResult = mParameters.find(name);
            if (findResult == mParameters.end())
                return;
            size_t typeIndex = findResult->second.index();
            if constexpr (std::is_same_v<std::remove_pointer_t<T>, Texture2D>)
            {
                if (typeIndex == size_t(ParameterType::Texture2D))
                    findResult->second = std::make_pair(osg::ref_ptr<Texture2D>(value), std::get<Texture2DUnitPair>(findResult->second).second);
            }
            else if constexpr (std::is_same_v<std::remove_pointer_t<T>, Texture2DArray>)
            {
                if (typeIndex == size_t(ParameterType::Texture2DArray))
                    findResult->second = std::make_pair(osg::ref_ptr<Texture2DArray>(value), std::get<Texture2DArrayUnitPair>(findResult->second).second);
            }
            else if constexpr (std::is_same_v<std::remove_pointer_t<T>, Texture3D>)
            {
                if (typeIndex == size_t(ParameterType::Texture3D))
                    findResult->second = std::make_pair(osg::ref_ptr<Texture3D>(value), std::get<Texture3DUnitPair>(findResult->second).second);
            }
            else if constexpr (std::is_same_v<std::remove_pointer_t<T>, TextureCubemap>)
            {
                if (typeIndex == size_t(ParameterType::TextureCubemap))
                    findResult->second = std::make_pair(osg::ref_ptr<TextureCubemap>(value), std::get<TextureCubemapUnitPair>(findResult->second).second);
            }
            else
            {
                if (findResult != mParameters.end() && findResult->second.index() == variant_index<T, ParameterValue>())
                    findResult->second = value;
            }
        }

        Parameters::const_iterator removeParameter(const std::string& name)
        {
            auto findResult = mParameters.find(name);
            if (findResult != mParameters.end())
                return mParameters.erase(findResult);
            return mParameters.end();
        }

        const Parameters& getParameters() const
        {
            return mParameters;
        }

        void apply() const;

    protected:
        Parameters mParameters;
        std::string mSource;

        int assignTextureUnit()
        {
            std::unordered_set<int> unavailableTextureUnit;
            for (const auto& param : mParameters)
            {
                switch (param.second.index())
                {
                case size_t(ParameterType::Texture2D):
                    unavailableTextureUnit.insert(std::get<Texture2DUnitPair>(param.second).second);
                    break;
                case size_t(ParameterType::Texture2DArray):
                    unavailableTextureUnit.insert(std::get<Texture2DArrayUnitPair>(param.second).second);
                    break;
                case size_t(ParameterType::Texture3D):
                    unavailableTextureUnit.insert(std::get<Texture3DUnitPair>(param.second).second);
                    break;
                case size_t(ParameterType::TextureCubemap):
                    unavailableTextureUnit.insert(std::get<TextureCubemapUnitPair>(param.second).second);
                    break;
                default:
                    break;
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
        template <> inline Type* Reflection::createType<Shader::ParameterType>()
        {
            Enumeration* enumeration = new TEnumeration<Shader::ParameterType>("Shader::ParameterType", {
                {"Bool", Shader::ParameterType::Bool},
                {"Int", Shader::ParameterType::Int},
                {"Float", Shader::ParameterType::Float},
                {"Vec2f", Shader::ParameterType::Vec2f},
                {"Vec3f", Shader::ParameterType::Vec3f},
                {"Vec4f", Shader::ParameterType::Vec4f},
                {"Texture2D", Shader::ParameterType::Texture2D},
                {"Texture2DArray", Shader::ParameterType::Texture2DArray},
                {"Texture3D", Shader::ParameterType::Texture3D},
                {"TextureCubemap", Shader::ParameterType::TextureCubemap},
            });
            return enumeration;
        }

        template <>
        inline Type* Reflection::createType<Shader>()
        {
            Class* clazz = new TClass<Shader>("Shader");
            clazz->addProperty("Parameters", &Shader::mParameters);
            clazz->addProperty("Source", &Shader::mSource);
            return clazz;
        }
    }
}
