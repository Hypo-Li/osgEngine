#include "Component.h"
#include "Entity.h"

namespace xxx
{
    Component::Component() :
        mEntity(nullptr),
        mOsgComponentGroup(new osg::Group)
    {

    }

    Component::Component(const Component& other) :
        mEntity(nullptr),
        mOsgComponentGroup(new osg::Group)
    {

    }

    namespace refl
    {
        template <> Type* Reflection::createType<Component>()
        {
            Class* clazz = new ClassInstance<Component>("Component");
            clazz->addProperty("Entity", &Component::mEntity);
            return clazz;
        }
    }
}
