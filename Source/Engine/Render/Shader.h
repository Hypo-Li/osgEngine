#pragma once
#include "Texture.h"

#include <osg/Shader>
#include <variant>

namespace xxx
{
    class Shader : public Object
    {
        friend class refl::Reflection;
    public:
        virtual refl::Class* getClass() const
        {
            return static_cast<refl::Class*>(refl::Reflection::getType<Shader>());
        }
    public:
        Shader() = default;

    private:
        using ShaderParameter = std::variant<bool, int, float, osg::Vec2f, osg::Vec3f, osg::Vec4f, osg::ref_ptr<Texture>>;
        std::vector<std::pair<std::string, ShaderParameter>> mParameters;
        std::string mSource;
        osg::ref_ptr<osg::Shader> mOsgShader;
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
            sRegisteredClassMap.emplace("Shader", clazz);
            return clazz;
        }
    }
}
