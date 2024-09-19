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

    private:
        std::unordered_map<std::string, osg::ref_ptr<Asset>> mAssets;
    };
}
