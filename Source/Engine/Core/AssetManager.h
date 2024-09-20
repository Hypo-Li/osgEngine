#pragma once
#include "Asset.h"

namespace xxx
{
    class AssetManager
    {
    public:
        static AssetManager& get()
        {
            static AssetManager assetManager;
            return assetManager;
        }

        Asset* createAsset(Object* rootObject, const std::string& path);

        Asset* getAsset(const std::string& path);

        Asset* getAsset(Guid guid);

    private:
        std::vector<osg::ref_ptr<Asset>> mAssets;
        std::unordered_map<std::string, Asset*> mPathAssetMap;
        std::unordered_map<Guid, Asset*> mGuidAssetMap;
    };
}
