#include "Engine/Core/Entity.h"
#include "Engine/Core/Component.h"
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
        sRegisteredClassMap.emplace("Test", clazz);
        return clazz;
    }

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

    outputPropertiesInfo(testClass);

    return 0;
}
