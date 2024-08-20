#pragma once
#include "Texture.h"

#include <osg/Shader>
#include <variant>

namespace xxx
{
    class Shader : public Object
    {
        friend class refl::Reflection;
        static Object* createInstance()
        {
            return new Shader;
        }
    public:
        virtual refl::Class* getClass() const
        {
            return static_cast<refl::Class*>(refl::Reflection::getType<Shader>());
        }
        static const Shader* getDefaultObject()
        {
            static osg::ref_ptr<Shader> defaultObject = new Shader;
            return defaultObject.get();
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
            Struct* structure = new Struct("osg::Vec2f", sizeof(osg::Vec2f));
            structure->addProperty("x",
                std::function<void(osg::Vec2f&, float&)>([](osg::Vec2& obj, float& v) { v = obj._v[0]; }),
                std::function<void(osg::Vec2f&, const float&)>([](osg::Vec2& obj, const float& v) {obj._v[0] = v; })
            );
            structure->addProperty("y",
                std::function<void(osg::Vec2f&, float&)>([](osg::Vec2& obj, float& v) { v = obj._v[1]; }),
                std::function<void(osg::Vec2f&, const float&)>([](osg::Vec2& obj, const float& v) {obj._v[1] = v; })
            );
            return structure;
        }

        template <>
        inline Type* Reflection::createType<osg::Vec3f>()
        {
            Struct* structure = new Struct("osg::Vec3f", sizeof(osg::Vec3f));
            structure->addProperty("x",
                std::function<void(osg::Vec3f&, float&)>([](osg::Vec3f& obj, float& v) { v = obj._v[0]; }),
                std::function<void(osg::Vec3f&, const float&)>([](osg::Vec3f& obj, const float& v) {obj._v[0] = v; })
            );
            structure->addProperty("y",
                std::function<void(osg::Vec3f&, float&)>([](osg::Vec3f& obj, float& v) { v = obj._v[1]; }),
                std::function<void(osg::Vec3f&, const float&)>([](osg::Vec3f& obj, const float& v) {obj._v[1] = v; })
            );
            structure->addProperty("z",
                std::function<void(osg::Vec3f&, float&)>([](osg::Vec3f& obj, float& v) { v = obj._v[2]; }),
                std::function<void(osg::Vec3f&, const float&)>([](osg::Vec3f& obj, const float& v) {obj._v[2] = v; })
            );
            return structure;
        }

        template <>
        inline Type* Reflection::createType<osg::Vec4f>()
        {
            Struct* structure = new Struct("osg::Vec4f", sizeof(osg::Vec4f));
            structure->addProperty("x",
                std::function<void(osg::Vec4f&, float&)>([](osg::Vec4f& obj, float& v) { v = obj._v[0]; }),
                std::function<void(osg::Vec4f&, const float&)>([](osg::Vec4f& obj, const float& v) {obj._v[0] = v; })
            );
            structure->addProperty("y",
                std::function<void(osg::Vec4f&, float&)>([](osg::Vec4f& obj, float& v) { v = obj._v[1]; }),
                std::function<void(osg::Vec4f&, const float&)>([](osg::Vec4f& obj, const float& v) {obj._v[1] = v; })
            );
            structure->addProperty("z",
                std::function<void(osg::Vec4f&, float&)>([](osg::Vec4f& obj, float& v) { v = obj._v[2]; }),
                std::function<void(osg::Vec4f&, const float&)>([](osg::Vec4f& obj, const float& v) {obj._v[2] = v; })
            );
            structure->addProperty("w",
                std::function<void(osg::Vec4f&, float&)>([](osg::Vec4f& obj, float& v) { v = obj._v[3]; }),
                std::function<void(osg::Vec4f&, const float&)>([](osg::Vec4f& obj, const float& v) {obj._v[3] = v; })
            );
            return structure;
        }

        template <>
        inline Type* Reflection::createType<Shader>()
        {
            Class* clazz = new Class("Shader", sizeof(Shader), Shader::createInstance);
            clazz->setBaseClass(dynamic_cast<Class*>(Reflection::getType<Object>()));
            Property* propParameters = clazz->addProperty("Parameters", &Shader::mParameters);
            Property* propSource = clazz->addProperty("Source", &Shader::mSource);
            sRegisteredClassMap.emplace("Shader", clazz);
            return clazz;
        }
    }
}
