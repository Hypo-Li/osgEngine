#include "AssetManager.h"

namespace xxx
{
    using namespace refl;

    Asset* AssetManager::createAsset(Object* rootObject, const std::string& assetPath)
    {
        Asset* asset = new Asset(assetPath);
        asset->setRootObject(rootObject);
        return asset;
    }

    Asset* AssetManager::getAsset(const std::string& path)
    {
        auto findResult = mAssets.find(path);
        if (findResult != mAssets.end())
            return findResult->second;
        return nullptr;
    }
}
