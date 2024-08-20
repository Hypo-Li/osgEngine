#pragma once
#include <Engine/Core/Serializable.h>

#include <osg/GL>
#include <osg/Array>
#include <osg/Texture>

#define LITTLE_ENDIAN 0
#define BIG_ENDIAN 1
#define BYTE_ORDER LITTLE_ENDIAN

namespace xxx
{
    class AssetManager;

    struct AssetMeta;
    class Asset : public osg::Referenced, public ISerializable
    {
        friend class AssetManager;
    public:
        enum class Type : uint32_t
        {
            Unknow,
            Texture,
            MaterialTemplate,
            MaterialInstance,
            StaticMesh,
        };

        Asset(const std::string& path);
        virtual ~Asset() = default;

        Type getType()
        {
            return _meta.type;
        }

        const std::string_view getPath()
        {
            return _path;
        }

        void load()
        {
            
        }

        void store() const
        {
            // magic
            // type
            // 
            // refAssets GUID
            // jsonSize
            // binarySize
        }


    protected:
        AssetMeta _meta;
        std::string _path;
    };
}
