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

protected:
    osg::ref_ptr<osg::Group> mOsgImplGroup;
};

namespace xxx::refl
{
    template <> Type* Reflection::createType<TestComponent>()
    {
        Class* clazz = new ClassInstance<TestComponent>("TestComponent");
        return clazz;
    }
}

int main()
{
    /*Entity* entity = new Entity;
    entity->addComponent(new TestComponent);
    entity->addChild(new Entity);
    AssetSaver* assetSaver = new AssetSaver(nullptr);
    assetSaver->serialize(entity);*/

    Shader* shader = new Shader;
    shader->addParameter("BaseColor", osg::Vec3f(0.8, 0.8, 0.8));
    shader->addParameter("Roughness", 0.5f);
    shader->addParameter("Metallic", 0.0f);
    shader->addParameter("Specular", 0.5f);

    {
        Asset* shaderAsset = AssetManager::get().createAsset(shader, TEMP_DIR "shader.xast");
        shaderAsset->save();
    }


    {
        Asset* shaderAsset = AssetManager::get().createAsset(nullptr, TEMP_DIR "shader.xast");
        shaderAsset->load();
        Object* object = shaderAsset->getRootObject();
        bool compareResult = object->getClass()->compare(shader, object);
        std::cout << compareResult << std::endl;
    }

    return 0;
}
