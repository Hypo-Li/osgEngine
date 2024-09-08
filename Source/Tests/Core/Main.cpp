#include "Engine/Core/Entity.h"
#include "Engine/Core/Component.h"
#include "Engine/Render/Shader.h"
#include "Engine/Core/Asset.h"
#include "Engine/Core/Prefab.h"
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
    Prefab* prefab = new Prefab;
    {
        Entity* entity = new Entity("Hello");
        entity->appendComponent(new TestComponent);
        Entity* entityChild0 = new Entity("World");
        entity->appendChild(entityChild0);
        prefab->appendPackedEntity(entity);
    }

    Asset* asset = new Asset;
    asset->setRootObject(prefab);

    osg::ref_ptr<Entity> ent = new Prefab;
    Class* clazz = ent->getClass();

    Prefab* pf = dynamic_cast<Prefab*>(ent.get());
    pf->setAsset(asset);
    pf->syncWithAsset();
    ent = Prefab::unpack(pf);

    return 0;
}
