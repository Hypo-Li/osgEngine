#pragma once
#include "Reflection/Reflection.h"

#include <osg/Vec2f>
#include <osg/Vec3f>
#include <osg/Vec4f>

namespace xxx::refl
{
    template <>
    inline Type* Reflection::createType<osg::Vec2f>()
    {
        Struct* structure = new StructInstance<osg::Vec2f>("osg::Vec2f");
        structure->addProperty<0>("x", &osg::Vec2f::_v);
        structure->addProperty<1>("y", &osg::Vec2f::_v);
        return structure;
    }

    template <>
    inline Type* Reflection::createType<osg::Vec3f>()
    {
        Struct* structure = new StructInstance<osg::Vec3f>("osg::Vec3f");
        structure->addProperty<0>("x", &osg::Vec2f::_v);
        structure->addProperty<1>("y", &osg::Vec2f::_v);
        structure->addProperty<2>("z", &osg::Vec2f::_v);
        return structure;
    }

    template <>
    inline Type* Reflection::createType<osg::Vec4f>()
    {
        Struct* structure = new StructInstance<osg::Vec4f>("osg::Vec4f");
        structure->addProperty<0>("x", &osg::Vec2f::_v);
        structure->addProperty<1>("y", &osg::Vec2f::_v);
        structure->addProperty<2>("z", &osg::Vec2f::_v);
        structure->addProperty<3>("w", &osg::Vec2f::_v);
        return structure;
    }
}
