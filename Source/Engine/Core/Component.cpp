#include "Component.h"

namespace xxx
{
    Component::Component() :
        mOwner(nullptr),
        mOsgComponentGroup(new osg::Group)
    {

    }
}
