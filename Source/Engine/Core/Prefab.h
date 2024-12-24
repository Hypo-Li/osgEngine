#pragma once
#include "Entity.h"
#include "Asset.h"
namespace xxx
{
    class Prefab : public Object
    {
        REFLECT_CLASS(Prefab)
    public:
        Prefab();
        Prefab(const Prefab& other);
        virtual ~Prefab() = default;

        Entity* generateEntity() const
        {
            return new Entity(*mRootEntity, true);
        }

    protected:
        osg::ref_ptr<Entity> mRootEntity;
    };

    namespace refl
    {
        template<> Type* Reflection::createType<Prefab>();
    }
}
