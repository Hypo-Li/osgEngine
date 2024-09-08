#include "Component.h"
#include "Entity.h"

namespace xxx
{
    Component::Component() :
        mOwner(nullptr),
        mOsgComponentGroup(new osg::Group)
    {

    }

    Component::Component(const Component& other) :
        mOwner(nullptr),
        mOsgComponentGroup(new osg::Group)
    {

    }
}
