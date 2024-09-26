#pragma once
#include "Asset.h"
#include "Context.h"

#include <iostream>

namespace xxx
{
    class AssetManager
    {
        friend class Asset;
    public:
        AssetManager()
        {
            const std::filesystem::path& engineAssetPath = Context::get().getEngineAssetPath();
            for (const auto& file : std::filesystem::recursive_directory_iterator(engineAssetPath))
            {
                if (file.is_regular_file())
                {
                    createAsset(nullptr, file.path().string());
                }
            }
            return;
        }

        static AssetManager& get()
        {
            static AssetManager assetManager;
            return assetManager;
        }

        Asset* createAsset(Object* rootObject, const std::string& path);

        Asset* getAsset(const std::string& path);

        Asset* getAsset(Guid guid);

    private:
        std::unordered_set<osg::ref_ptr<Asset>> mAssets;
        std::unordered_map<std::string, Asset*> mPathAssetMap;
        std::unordered_map<Guid, Asset*> mGuidAssetMap;
    };
}
