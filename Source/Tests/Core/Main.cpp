#include "Engine/Core/Entity.h"
#include "Engine/Core/Component.h"
#include "Engine/Render/Shader.h"
#include <osg/Vec2f>
#include <osg/Vec3f>
#include <osg/Vec4f>
#include <iostream>

using namespace xxx;
using namespace xxx::refl;

enum class Color
{
    Red,
    Green,
    Blue,
};

class Test : public Object
{
    friend class Reflection;
public:
    virtual refl::Class* getClass() const
    {
        return static_cast<refl::Class*>(refl::Reflection::getType<Test>());
    }
public:
    std::vector<Object*> mObjects;
    osg::Vec2f mVec2;
    std::set<int> mIntSet;
    std::map<std::string, double> mStringDoubleMap;
    Color mColor = Color::Red;
    int mInt = 0;
    float mFloat = 1.f;
    osg::ref_ptr<Test> mTest;

    void setP(float newP) { p = newP; }
    float getP() const { return p; }

private:
    float p;

};

namespace xxx::refl
{
    template <>
    inline Type* Reflection::createType<Color>()
    {
        Enum* enumerate = new EnumInstance<Color>(
            "Color",
            {
                {"Red", Color::Red},
                {"Green", Color::Green},
                {"Blue", Color::Blue},
            }
        );
        return enumerate;
    }

    template <>
    inline Type* Reflection::createType<Test>()
    {
        Class* clazz = new ClassInstance<Test>("Test");
        Property* propObjects = clazz->addProperty("Objects", &Test::mObjects);
        Property* propVec2 = clazz->addProperty("Vec2", &Test::mVec2);
        propVec2->addMetadata(MetaKey::ClampMin, 0);
        propVec2->addMetadata(MetaKey::ClampMax, 1.0f);
        propVec2->addMetadata(MetaKey::DisplayName, "MyVec2");
        propVec2->addMetadata(MetaKey::ToolTip, "This is a demo vec2");
        Property* propP = clazz->addProperty("p", &Test::p);
        Property* propIntSet = clazz->addProperty("IntSet", &Test::mIntSet);
        Property* propStringDoubleMap = clazz->addProperty("StringDoubleMap", &Test::mStringDoubleMap);
        clazz->addProperty("Color", &Test::mColor);
        clazz->addProperty("Int", &Test::mInt);
        clazz->addProperty("Float", &Test::mFloat);
        clazz->addProperty("Test", &Test::mTest);
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
        std::cout << prop->getDeclaredType()->getName() << " " << prop->getName() << std::endl;
    }
}

struct StructWithoutEqualOperator
{
    float a;
    int b;
};

struct StructWithEqualOperator
{
    float a;
    int b;

    bool operator==(const StructWithEqualOperator& rhs) const
    {
        return a == rhs.a && b == rhs.b;
    }
};

namespace xxx::refl
{
    template <>
    inline Type* Reflection::createType<StructWithoutEqualOperator>()
    {
        Struct* structure = new StructInstance<StructWithoutEqualOperator>("StructWithoutEqualOperator");
        structure->addProperty("a", &StructWithoutEqualOperator::a);
        structure->addProperty("b", &StructWithoutEqualOperator::b);
        return structure;
    }

    template <>
    inline Type* Reflection::createType<StructWithEqualOperator>()
    {
        Struct* structure = new StructInstance<StructWithEqualOperator>("StructWithEqualOperator");
        structure->addProperty("a", &StructWithEqualOperator::a);
        structure->addProperty("b", &StructWithEqualOperator::b);
        return structure;
    }
}

void serialize_test(Object* object)
{
    Class* clazz = object->getClass();
    const Object* defaultObject = clazz->getDefaultObject();
    std::vector<Property*> serializedProperties;
    Class* baseClass = clazz;
    while (baseClass)
    {
        for (Property* prop : baseClass->getProperties())
        {
            if (!prop->compare(defaultObject, object))
                serializedProperties.emplace_back(prop);
        }
        baseClass = baseClass->getBaseClass();
    }
    size_t propertyCount = serializedProperties.size();
    for (Property* prop : serializedProperties)
    {
        std::cout << prop->getName() << ": " << prop->getDeclaredType()->getName() << std::endl;
    }
    return;
}

void compare_test()
{
    StructWithoutEqualOperator a1 = { 1.0f, 2 }, a2 = {2.0f, 3};
    StructWithEqualOperator b1 = { 1.0f, 2 }, b2 = { 2.0f, 3 };
    Type* tA = Reflection::getType<decltype(a1)>();
    Type* tB = Reflection::getType<decltype(b1)>();
    bool retA1 = tA->compare(&a1, &a2);
    bool retA2 = tA->compare(&a1, &a1);
    bool retB1 = tB->compare(&b1, &b2);
    bool retB2 = tB->compare(&b1, &b1);

    Test* t0 = new Test;

    Type* testType = Reflection::getType<Test>();
    Test* t1 = static_cast<Test*>(testType->newInstance());
    
    t1->mIntSet.insert(5);
    t1->mVec2.x() = 1.0f;
    t1->mColor = Color::Green;
    t1->mInt = 2;
    t1->mFloat = 4.5f;
    //t1->mTest = t0;

    Class* shaderClass = Reflection::getClass<Shader>();
    Property* propParameters = shaderClass->getProperty("Parameters");
    Type* parametersType = propParameters->getDeclaredType();

    serialize_test(t1);

    return;
}

int main()
{
    compare_test();
    /*Class* testClass = dynamic_cast<Class*>(Reflection::getType<Test>());
    Property* propObjects = testClass->getProperty("Objects");
    Property* propVec2 = testClass->getProperty("Vec2");
    std::optional<std::string_view> displayName = propVec2->getMetadata<std::string_view>(MetaKey::DisplayName);
    std::optional<std::string_view> category = propVec2->getMetadata<std::string_view>(MetaKey::Category);
    StdVector* stdVectorObjects = dynamic_cast<StdVector*>(propObjects->getDeclaredType());

    const Object* testDefaultObject = testClass->getDefaultObject();
    Object* obj = new Object;
    Test t;
    t.mIntSet.insert(2);
    t.mIntSet.insert(3);
    t.mIntSet.insert(8);
    StdSet* stdSet = dynamic_cast<StdSet*>(testClass->getProperty("IntSet")->getDeclaredType());
    auto ptrs = stdSet->getElementPtrs(&(t.mIntSet));
    t.mIntSet.insert(5);
    for (auto ptr : ptrs)
    {
        *(int*)ptr = 1;

    }
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

    outputPropertiesInfo(testClass);*/

    return 0;
}
