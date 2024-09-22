#include "Engine/Core/Entity.h"
#include "Engine/Core/Component.h"
#include "Engine/Render/Shader.h"
#include "Engine/Core/Asset.h"
#include "Engine/Core/Prefab.h"
#include "Engine/Core/AssetManager.h"
#include <osg/Vec2f>
#include <osg/Vec3f>
#include <osg/Vec4f>
#include <iostream>

using namespace xxx;
using namespace xxx::refl;

class TestComponent : public Component
{
    friend class refl::Reflection;
public:
    virtual refl::Class* getClass() const
    {
        return static_cast<refl::Class*>(refl::Reflection::getType<TestComponent>());
    }

public:
    TestComponent() :
        mOsgImplGroup(new osg::Group)
    {
        mOsgComponentGroup->addChild(mOsgImplGroup);
    }

    TestComponent(const TestComponent& other) :
        mOsgImplGroup(new osg::Group(*other.mOsgImplGroup))
    {
        mOsgComponentGroup->addChild(mOsgImplGroup);
    }

    virtual TestComponent* clone() const override
    {
        return new TestComponent(*this);
    }

    virtual Type getType() const override
    {
        return Type::MeshRenderer;
    }

    void setShader(Shader* shader)
    {
        mShader = shader;
    }

protected:
    osg::ref_ptr<Shader> mShader;

    osg::ref_ptr<osg::Group> mOsgImplGroup;
};

namespace xxx::refl
{
    template <> Type* Reflection::createType<TestComponent>()
    {
        Class* clazz = new ClassInstance<TestComponent, Component>("TestComponent");
        clazz->addProperty("TestShader", &TestComponent::mShader);
        sRegisteredClassMap.emplace("TestComponent", clazz);
        return clazz;
    }
}

int main()
{
    Shader* shader = new Shader;
    shader->addParameter("BaseColor", osg::Vec3f(0.8, 0.8, 0.8));
    shader->addParameter("Roughness", 0.5f);
    shader->addParameter("Metallic", 0.0f);
    shader->addParameter("Specular", 0.5f);
    Class* clazz = shader->getClass();
    Method* setParameterFloatMethod = clazz->getMethod("setParameter<float>");
    setParameterFloatMethod->invoke(shader, std::string("Roughness"), 0.01f);
    {
        Asset* shaderAsset = AssetManager::get().createAsset(shader, TEMP_DIR "Shader.xast");
        shaderAsset->save();
    }

    Entity* entity = new Entity;
    TestComponent* testComponent = new TestComponent;
    testComponent->setShader(shader);
    entity->addComponent(testComponent);
    entity->addChild(new Entity);
    {
        Asset* entityAsset = AssetManager::get().createAsset(entity, TEMP_DIR "Entity.xast");
        entityAsset->save();
    }

    {
        Asset* entityAsset = AssetManager::get().createAsset(nullptr, TEMP_DIR "Entity.xast");
        entityAsset->load();
        Object* object = entityAsset->getRootObject();
    }

    return 0;
}
