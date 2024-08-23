#include "Engine/Core/Entity.h"
#include "Engine/Core/Component.h"
#include "Engine/Render/Shader.h"
#include <osg/Vec2f>
#include <osg/Vec3f>
#include <osg/Vec4f>
#include <iostream>

using namespace xxx;
using namespace xxx::refl;

class Test : public Object
{
    friend class Reflection;
    static Object* createInstance()
    {
        return new Test;
    }
public:
    virtual refl::Class* getClass() const
    {
        return static_cast<refl::Class*>(refl::Reflection::getType<Test>());
    }
public:
    std::vector<Object*> mObjects;
    osg::Vec2f mVec2;

    void setP(float newP) { p = newP; }
    float getP() const { return p; }

private:
    float p;

};

enum class Color
{
    Red,
    Green,
    Blue,
};

namespace xxx::refl
{
    template <>
    inline Type* Reflection::createType<Color>()
    {
        using EnumValue = std::pair<std::string_view, Color>;
        Enum* enumerate = new Enum(
            "Color",
            {
                EnumValue("Red", Color::Red),
                EnumValue("Green", Color::Green),
                EnumValue("Blue", Color::Blue),
            }
        );
        return enumerate;
    }

    template <>
    inline Type* Reflection::createType<Test>()
    {
        Class* clazz = new Class("Test", sizeof(Test), Test::createInstance);
        clazz->setBaseClass(dynamic_cast<Class*>(Reflection::getType<Object>()));
        Property* propObjects = clazz->addProperty("Objects", &Test::mObjects);
        Property* propVec2 = clazz->addProperty("Vec2", &Test::mVec2);
        propVec2->addMetadata(MetaKey::ClampMin, 0);
        propVec2->addMetadata(MetaKey::ClampMax, 1.0f);
        propVec2->addMetadata(MetaKey::DisplayName, "MyVec2");
        propVec2->addMetadata(MetaKey::ToolTip, "This is a demo vec2");
        Property* propP = clazz->addProperty("p",
            std::function<void(const Test&, float&)>([](const Test& obj, float& v) { v = obj.getP(); }),
            std::function<void(Test&, const float&)>([](Test& obj, const float& v) { obj.setP(v); })
        );
        sRegisteredClassMap.emplace("Test", clazz);
        return clazz;
    }

}

void outputPropertiesInfo(Class* clazz)
{
    if (clazz->getBaseClass())
        outputPropertiesInfo(clazz->getBaseClass());
    for (Property* prop : clazz->getProperties())
    {
        std::cout << prop->getType()->getName() << " " << prop->getName() << std::endl;
    }
}

int main()
{
    Class* testClass = dynamic_cast<Class*>(Reflection::getType<Test>());
    Property* propObjects = testClass->getProperty("Objects");
    Property* propVec2 = testClass->getProperty("Vec2");
    std::optional<std::string_view> displayName = propVec2->getMetadata<std::string_view>(MetaKey::DisplayName);
    std::optional<std::string_view> category = propVec2->getMetadata<std::string_view>(MetaKey::Category);
    StdVector* stdVectorObjects = dynamic_cast<StdVector*>(propObjects->getType());

    const Object* testDefaultObject = testClass->getDefaultObject();
    Object* obj = new Object;
    Test t;
    stdVectorObjects->appendElement(propObjects->getValuePtr(&t), obj);

    Entity* entity0 = new Entity;
    Entity* entity1 = new Entity;
    entity0->appendChild(entity1);

    Struct* vec2fStruct = Reflection::getStruct<osg::Vec2f>();
    Struct* vec3fStruct = Reflection::getStruct<osg::Vec3f>();
    Struct* vec4fStruct = Reflection::getStruct<osg::Vec4f>();
    Property* propX = vec2fStruct->getProperty("x");
    osg::Vec2f v(1.0f, 2.0f);
    float x;
    propX->getValue(&v, &x);
    propX->setValue(&v, x + 1.0f);
    void* ptr = propX->getValuePtr(&v);

    Enum* colorEnum = dynamic_cast<Enum*>(Reflection::getType<Color>());

    Any any = 0.0f;

    Class* classShader = Reflection::getClass<Shader>();
    Class* classTexture = Reflection::getClass<Texture>();

    const Object* shaderDefaultObject = classShader->getDefaultObject();
    const Object* textureDefaultObject = classTexture->getDefaultObject();

    outputPropertiesInfo(testClass);

    return 0;
}
