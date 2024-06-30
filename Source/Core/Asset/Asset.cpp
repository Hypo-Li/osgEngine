#include "Asset.h"
#include <osg/Texture2D>
#include <osg/Texture2DArray>
#include <osg/Texture3D>
#include <osg/TextureCubeMap>

namespace xxx
{
    const std::unordered_map<GLenum, std::string> Asset::_sTextureTypeNameMap = {
        {GL_TEXTURE_2D, "Texture2D"},
        {GL_TEXTURE_2D_ARRAY, "Texture2DArray"},
        {GL_TEXTURE_3D, "Texture3D"},
        {GL_TEXTURE_CUBE_MAP, "TextureCubemap"}
    };

    const std::unordered_map<std::string, GLenum> Asset::_sTextureNameTypeMap = {
        {"Texture2D", GL_TEXTURE_2D},
        {"Texture2DArray", GL_TEXTURE_2D_ARRAY},
        {"Texture3D", GL_TEXTURE_3D},
        {"TextureCubemap", GL_TEXTURE_CUBE_MAP}
    };

    const std::unordered_map<osg::Array::Type, std::string> Asset::_sArrayTypeNameMap = {
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

    const std::unordered_map<std::string, osg::Array::Type> Asset::_sArrayNameTypeMap = {
        {"Int", osg::Array::Type::IntArrayType},
        {"Int2", osg::Array::Type::Vec2iArrayType},
        {"Int3", osg::Array::Type::Vec3iArrayType},
        {"Int4", osg::Array::Type::Vec4iArrayType},

        {"Uint", osg::Array::Type::UIntArrayType},
        {"Uint2", osg::Array::Type::Vec2uiArrayType},
        {"Uint3", osg::Array::Type::Vec3uiArrayType},
        {"Uint4", osg::Array::Type::Vec4uiArrayType},

        {"Float", osg::Array::Type::FloatArrayType},
        {"Float2", osg::Array::Type::Vec2ArrayType},
        {"Float2", osg::Array::Type::Vec3ArrayType},
        {"Float4", osg::Array::Type::Vec4ArrayType},

        {"Double", osg::Array::Type::DoubleArrayType},
        {"Double2", osg::Array::Type::Vec2dArrayType},
        {"Double2", osg::Array::Type::Vec3dArrayType},
        {"Double4", osg::Array::Type::Vec4dArrayType},

        {"Float4x4", osg::Array::Type::MatrixArrayType},
        {"Double4x4", osg::Array::Type::MatrixdArrayType}
    };

    osg::ref_ptr<osg::Texture> Asset::createTextureByType(GLenum type)
    {
        switch (type)
        {
        case GL_TEXTURE_2D:
            return new osg::Texture2D;
        case GL_TEXTURE_2D_ARRAY:
            return new osg::Texture2DArray;
        case GL_TEXTURE_3D:
            return new osg::Texture3D;
        case GL_TEXTURE_CUBE_MAP:
            return new osg::TextureCubeMap;
        default:
            return nullptr;
        }
    }

    osg::ref_ptr<osg::Array> Asset::createArrayByType(osg::Array::Type type)
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

    size_t Asset::getReferenceIndex(const std::string& assetPath, std::vector<std::string>& reference)
    {
        size_t index;
        auto it = std::find(reference.begin(), reference.end(), assetPath);
        if (it == reference.end())
        {
            index = reference.size();
            reference.push_back(assetPath);
        }
        else
        {
            index = std::distance(reference.begin(), it);
        }
        return index;
    }

}
