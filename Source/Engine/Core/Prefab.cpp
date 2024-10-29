#include "Prefab.h"

namespace xxx::refl
{
    template <> Type* Reflection::createType<Prefab>()
    {
        Class* clazz = new TClass<Prefab>("Prefab");
        clazz->addProperty("RootEntity", &Prefab::mRootEntity);
        return clazz;
    }
}

namespace xxx
{
    Prefab::Prefab() :
        mRootEntity(nullptr)
    {
        
    }

    Prefab::Prefab(const Prefab& other) :
        mRootEntity(other.mRootEntity)
    {
        // TODO: copy children
    }
}
