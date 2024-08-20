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
    static const Test* getDefaultObject()
    {
        static osg::ref_ptr<Test> defaultTest = new Test;
        return defaultTest.get();
    }
public:
    std::vector<Object*> mObjects;
    osg::Vec2f mVec2;
};

namespace xxx::refl
{
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
    
    Object* obj = new Object;
    Test t;
    stdVectorObjects->appendElement(propObjects->getValuePtr(&t), obj);

    Entity* entity0 = new Entity;
    Entity* entity1 = new Entity;
    entity0->appendChild(entity1);

    Struct* vec2fStruct = dynamic_cast<Struct*>(Reflection::getType<osg::Vec2f>());
    Property* propX = vec2fStruct->getProperty("x");
    osg::Vec2f v(1.0f, 2.0f);
    float x;
    propX->getValue(&v, &x);
    propX->setValue(&v, x + 1.0f);
    void* ptr = propX->getValuePtr(&v);

    Any any = 0.0f;

    Type* classShader = Reflection::getType<Shader>();
    Type* classTexture = Reflection::getType<Texture>();

    outputPropertiesInfo(testClass);

    return 0;
}
