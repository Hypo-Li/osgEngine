#pragma once
#include "Reflection/Reflection.h"

#include <osg/Vec2f>
#include <osg/Vec3f>
#include <osg/Vec4f>
#include <osg/Matrixf>
#include <osg/Vec2d>
#include <osg/Vec3d>
#include <osg/Vec4d>
#include <osg/Matrixd>

namespace xxx::refl
{
    template <> Type* Reflection::createType<osg::Vec2f>();

    template <> Type* Reflection::createType<osg::Vec2d>();

    template <> Type* Reflection::createType<osg::Vec3f>();

    template <> Type* Reflection::createType<osg::Vec3d>();

    template <> Type* Reflection::createType<osg::Vec4f>();

    template <> Type* Reflection::createType<osg::Vec4d>();

    template <> Type* Reflection::createType<osg::Matrixf>();

    template <> Type* Reflection::createType<osg::Matrixd>();
}
