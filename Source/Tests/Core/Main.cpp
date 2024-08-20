#include "Engine/Core/Entity.h"
#include "Engine/Core/Component.h"

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
};

namespace xxx::refl
{
    template <>
    inline Type* Reflection::createType<Test>()
    {
        Class* clazz = new Class("Test", sizeof(Test), Test::createInstance);
        clazz->setBaseClass(dynamic_cast<Class*>(Reflection::getType<Object>()));
        Property* propObjects = clazz->addProperty("Objects", &Test::mObjects);
        sRegisteredClassMap.emplace("Test", clazz);
        return clazz;
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
    return 0;
}
