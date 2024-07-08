#pragma once
#include "Asset.h"
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace xxx
{
    class AssetManager
    {
    public:
        //AssetManager() = default;
        //virtual ~AssetManager() = default;

        static Asset* loadAsset(const std::string& path);

        template <typename T>
        static T* loadAsset(const std::string& path)
        {
            return dynamic_cast<T*>(loadAsset(path));
        }

        static void storeAsset(const std::string& path, Asset* asset);

        static Asset::Type getAssetType(const std::string& path);

    private:
        static std::unordered_map<std::string, osg::ref_ptr<Asset>> _sAssetMap;

#if (BYTE_ORDER == BIG_ENDIAN)
        static constexpr uint32_t _sAssetMagic = 0x58415354; // XAST
#else
        static constexpr uint32_t _sAssetMagic = 0x54534158; // XAST
#endif

        static Asset* tryGetAssetFromCache(const std::string& path);
        
    };
}
