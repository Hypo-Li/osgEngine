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
}
