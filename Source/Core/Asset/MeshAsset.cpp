#include "MeshAsset.h"

namespace xxx
{

    const Asset::ConstBiMap<osg::Array::Type, std::string> MeshAsset::_sArrayTypeStringMap = {
        {osg::Array::Type::IntArrayType, "Int"},
        {osg::Array::Type::Vec2iArrayType, "Int2"},
        {osg::Array::Type::Vec3iArrayType, "Int3"},
        {osg::Array::Type::Vec4iArrayType, "Int4"},

        {osg::Array::Type::UIntArrayType, "Uint"},
        {osg::Array::Type::Vec2uiArrayType, "Uint2"},
        {osg::Array::Type::Vec3uiArrayType, "Uint3"},
        {osg::Array::Type::Vec4uiArrayType, "Uint4"},

        {osg::Array::Type::FloatArrayType, "Float"},
        {osg::Array::Type::Vec2ArrayType, "Float2"},
        {osg::Array::Type::Vec3ArrayType, "Float2"},
        {osg::Array::Type::Vec4ArrayType, "Float4"},

        {osg::Array::Type::DoubleArrayType, "Double"},
        {osg::Array::Type::Vec2dArrayType, "Double2"},
        {osg::Array::Type::Vec3dArrayType, "Double2"},
        {osg::Array::Type::Vec4dArrayType, "Double4"},

        {osg::Array::Type::MatrixArrayType, "Float4x4"},
        {osg::Array::Type::MatrixdArrayType, "Double4x4"}
    };

    osg::ref_ptr<osg::Array> MeshAsset::createArrayByType(osg::Array::Type type)
    {
        switch (type)
        {
        case osg::Array::Type::IntArrayType:
            return new osg::IntArray;
        case osg::Array::Type::Vec2iArrayType:
            return new osg::Vec2iArray;
        case osg::Array::Type::Vec3iArrayType:
            return new osg::Vec3iArray;
        case osg::Array::Type::Vec4iArrayType:
            return new osg::Vec4iArray;
        case osg::Array::Type::UIntArrayType:
            return new osg::UIntArray;
        case osg::Array::Type::Vec2uiArrayType:
            return new osg::Vec2uiArray;
        case osg::Array::Type::Vec3uiArrayType:
            return new osg::Vec3uiArray;
        case osg::Array::Type::Vec4uiArrayType:
            return new osg::Vec4uiArray;
        case osg::Array::Type::FloatArrayType:
            return new osg::FloatArray;
        case osg::Array::Type::Vec2ArrayType:
            return new osg::Vec2Array;
        case osg::Array::Type::Vec3ArrayType:
            return new osg::Vec3Array;
        case osg::Array::Type::Vec4ArrayType:
            return new osg::Vec4Array;
        case osg::Array::Type::DoubleArrayType:
            return new osg::DoubleArray;
        case osg::Array::Type::Vec2dArrayType:
            return new osg::Vec2dArray;
        case osg::Array::Type::Vec3dArrayType:
            return new osg::Vec3dArray;
        case osg::Array::Type::Vec4dArrayType:
            return new osg::Vec4dArray;
        case osg::Array::Type::MatrixArrayType:
            return new osg::MatrixfArray;
        case osg::Array::Type::MatrixdArrayType:
            return new osg::MatrixdArray;
        default:
            return nullptr;
        }
    }
}
