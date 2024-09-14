#pragma once
#include "Asset.h"

namespace xxx
{
    class AssetManager
    {
    public:
        static Asset* createAsset(Object* rootObject, const std::string& path);

        static Asset* getAsset(const std::string& path);

    private:
        static std::unordered_map<std::string, osg::ref_ptr<Asset>> mAssets;

        static void setObjectAssetRecursively(Object* object, Asset* asset);
    };
}
