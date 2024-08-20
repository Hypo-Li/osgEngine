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

        static void storeAsset(Asset* asset);

        static void unloadAsset(const std::string& path);

        static bool createAsset(const std::string& path, Asset::Type type);

        static bool moveAsset(const std::string& srcPath, const std::string& dstPath);

        static bool copyAsset(const std::string& srcPath, const std::string& dstPath);

        static bool deleteAsset(const std::string& path);

        static Asset::Type getAssetType(const std::string& path);

    private:

#if (BYTE_ORDER == BIG_ENDIAN)
        static constexpr uint32_t _sAssetMagic = 0x58415354; // XAST
#else
        static constexpr uint32_t _sAssetMagic = 0x54534158; // XAST
#endif

        static Asset* tryGetAssetFromCache(const std::string& path);

        static std::unordered_map<std::string, AssetMeta> _sAvailableAssets;
        static std::unordered_map<std::string, osg::ref_ptr<Asset>> _sLoadedAssets;
        
    };
}
