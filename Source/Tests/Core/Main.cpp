#include "Engine/Core/Entity.h"
#include "Engine/Core/Component.h"
#include "Engine/Render/Shader.h"
#include "Engine/Render/Mesh.h"
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
    REFLECT_CLASS(TestComponent)
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
        getClassMap().emplace("TestComponent", clazz);
        return clazz;
    }
}

struct TestStruct
{
    int a;
    float b;
    char c;
};

int main()
{
    /*AssetManager::get();
    Shader* shader = new Shader;
    shader->addParameter("BaseColor", osg::Vec3f(0.8, 0.8, 0.8));
    shader->addParameter("Roughness", 0.5f);
    shader->addParameter("Metallic", 0.0f);
    shader->addParameter("Specular", 0.5f);
    Class* clazz = shader->getClass();
    Method* setParameterFloatMethod = clazz->getMethod("setParameter<float>");
    setParameterFloatMethod->invoke(shader, std::string("Roughness"), 0.01f);
    {
        Asset* shaderAsset = AssetManager::get().createAsset(shader, ASSET_DIR "Shader.xast");
        shaderAsset->save();
    }

    Entity* entity = new Entity;
    TestComponent* testComponent = new TestComponent;
    testComponent->setShader(shader);
    entity->addComponent(testComponent);
    entity->addChild(new Entity);
    {
        Asset* entityAsset = AssetManager::get().createAsset(entity, ASSET_DIR "Entity.xast");
        entityAsset->save();
    }

    {
        Asset* entityAsset = AssetManager::get().getAsset(ASSET_DIR "Entity.xast");
        entityAsset->load();
        Object* object = entityAsset->getRootObject();
        std::string guid = object->getGuid().toString();
        std::cout << guid << std::endl;
    }*/

    /*Texture2D* texture2d = new Texture2D(TEMP_DIR "awesomeface.png");
    Asset* textureAsset = AssetManager::get().createAsset(texture2d, ASSET_DIR "Texture/Awesomeface.xast");
    textureAsset->save();*/

    /*AssetManager& am = AssetManager::get();

    Asset* shaderAsset = am.getAsset(ASSET_DIR "Shader.xast");
    shaderAsset->load();
    Shader* shader = shaderAsset->getRootObject<Shader>();

    Asset* textureAsset = am.getAsset(ASSET_DIR "Texture/Awesomeface.xast");
    textureAsset->load();
    Texture* texture = textureAsset->getRootObject<Texture>();

    shader->addParameter("BaseColorTexture", texture);
    shaderAsset->forceSave();*/

    Mesh* mesh = new Mesh(TEMP_DIR "test.fbx");

    return 0;
}
