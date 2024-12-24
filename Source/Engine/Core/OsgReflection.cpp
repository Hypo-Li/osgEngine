#include "OsgReflection.h"

struct ReflVec2f
{
    float x, y;
};

struct ReflVec2d
{
    double x, y;
};

struct ReflVec3f
{
    float x, y, z;
};

struct ReflVec3d
{
    double x, y, z;
};

struct ReflVec4f
{
    float x, y, z, w;
};

struct ReflVec4d
{
    double x, y, z, w;
};

struct ReflMatrixf
{
    float m00, m10, m20, m30;
    float m01, m11, m21, m31;
    float m02, m12, m22, m32;
    float m03, m13, m23, m33;
};

struct ReflMatrixd
{
    double m00, m10, m20, m30;
    double m01, m11, m21, m31;
    double m02, m12, m22, m32;
    double m03, m13, m23, m33;
};

namespace xxx::refl
{
    template <>
    Type* Reflection::createType<osg::Vec2f>()
    {
        Structure* structure = new TStructure<ReflVec2f>("osg::Vec2f");
        structure->addProperty("x", &ReflVec2f::x);
        structure->addProperty("y", &ReflVec2f::y);
        return structure;
    }
    template <>
    Type* Reflection::createType<osg::Vec2d>()
    {
        Structure* structure = new TStructure<ReflVec2d>("osg::Vec2d");
        structure->addProperty("x", &ReflVec2d::x);
        structure->addProperty("y", &ReflVec2d::y);
        return structure;
    }

    template <>
    Type* Reflection::createType<osg::Vec3f>()
    {
        Structure* structure = new TStructure<ReflVec3f>("osg::Vec3f");
        structure->addProperty("x", &ReflVec3f::x);
        structure->addProperty("y", &ReflVec3f::y);
        structure->addProperty("z", &ReflVec3f::z);
        return structure;
    }

    template <>
    Type* Reflection::createType<osg::Vec3d>()
    {
        Structure* structure = new TStructure<ReflVec3d>("osg::Vec3d");
        structure->addProperty("x", &ReflVec3d::x);
        structure->addProperty("y", &ReflVec3d::y);
        structure->addProperty("z", &ReflVec3d::z);
        return structure;
    }

    template <>
    Type* Reflection::createType<osg::Vec4f>()
    {
        Structure* structure = new TStructure<ReflVec4f>("osg::Vec4f");
        structure->addProperty("x", &ReflVec4f::x);
        structure->addProperty("y", &ReflVec4f::y);
        structure->addProperty("z", &ReflVec4f::z);
        structure->addProperty("w", &ReflVec4f::w);
        return structure;
    }

    template <>
    Type* Reflection::createType<osg::Vec4d>()
    {
        Structure* structure = new TStructure<ReflVec4d>("osg::Vec4d");
        structure->addProperty("x", &ReflVec4d::x);
        structure->addProperty("y", &ReflVec4d::y);
        structure->addProperty("z", &ReflVec4d::z);
        structure->addProperty("w", &ReflVec4d::w);
        return structure;
    }

    template <>
    Type* Reflection::createType<osg::Matrixf>()
    {

        Structure* structure = new TStructure<ReflMatrixf>("osg::Matrixf");

        structure->addProperty("m00", &ReflMatrixf::m00);
        structure->addProperty("m10", &ReflMatrixf::m10);
        structure->addProperty("m20", &ReflMatrixf::m20);
        structure->addProperty("m30", &ReflMatrixf::m30);

        structure->addProperty("m01", &ReflMatrixf::m01);
        structure->addProperty("m11", &ReflMatrixf::m11);
        structure->addProperty("m21", &ReflMatrixf::m21);
        structure->addProperty("m31", &ReflMatrixf::m31);

        structure->addProperty("m02", &ReflMatrixf::m02);
        structure->addProperty("m12", &ReflMatrixf::m12);
        structure->addProperty("m22", &ReflMatrixf::m22);
        structure->addProperty("m32", &ReflMatrixf::m32);

        structure->addProperty("m03", &ReflMatrixf::m03);
        structure->addProperty("m13", &ReflMatrixf::m13);
        structure->addProperty("m23", &ReflMatrixf::m23);
        structure->addProperty("m33", &ReflMatrixf::m33);

        return structure;
    }

    template <>
    Type* Reflection::createType<osg::Matrixd>()
    {

        Structure* structure = new TStructure<ReflMatrixd>("osg::Matrixd");

        structure->addProperty("m00", &ReflMatrixd::m00);
        structure->addProperty("m10", &ReflMatrixd::m10);
        structure->addProperty("m20", &ReflMatrixd::m20);
        structure->addProperty("m30", &ReflMatrixd::m30);

        structure->addProperty("m01", &ReflMatrixd::m01);
        structure->addProperty("m11", &ReflMatrixd::m11);
        structure->addProperty("m21", &ReflMatrixd::m21);
        structure->addProperty("m31", &ReflMatrixd::m31);

        structure->addProperty("m02", &ReflMatrixd::m02);
        structure->addProperty("m12", &ReflMatrixd::m12);
        structure->addProperty("m22", &ReflMatrixd::m22);
        structure->addProperty("m32", &ReflMatrixd::m32);

        structure->addProperty("m03", &ReflMatrixd::m03);
        structure->addProperty("m13", &ReflMatrixd::m13);
        structure->addProperty("m23", &ReflMatrixd::m23);
        structure->addProperty("m33", &ReflMatrixd::m33);

        return structure;
    }
}
