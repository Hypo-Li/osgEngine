#include "Prefab.h"

namespace xxx::refl
{
    template <> Type* Reflection::createType<Prefab>()
    {
        Class* clazz = new ClassInstance<Prefab>("Prefab");
        clazz->addProperty("PackedEntities", &Prefab::mPackedEntities);
        return clazz;
    }
}

namespace xxx
{
    Prefab::Prefab() :
        mOsgPackedEntitiesGroup(new osg::Group)
    {
        mOsgChildrenGroup->addChild(mOsgPackedEntitiesGroup);
    }

    Prefab::Prefab(const Prefab& other) :
        mOsgPackedEntitiesGroup(new osg::Group)
    {
        mOsgChildrenGroup->addChild(mOsgPackedEntitiesGroup);
    }
}
