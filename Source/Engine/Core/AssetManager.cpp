#include "AssetManager.h"

namespace xxx
{
    using namespace refl;

    Asset* AssetManager::createAsset(Object* rootObject, const std::string& path)
    {
        auto findResult = mPathAssetMap.find(path);
        if (findResult != mPathAssetMap.end())
        {
            return nullptr;
            /*Asset* asset = findResult->second;
            mPathAssetMap.erase(findResult);
            mGuidAssetMap.erase(asset->getGuid());
            mAssets.erase(asset);*/
        }

        Asset* asset = new Asset(path);
        asset->setRootObject(rootObject);
        mAssets.emplace(asset);
        mPathAssetMap.emplace(path, asset);
        mGuidAssetMap.emplace(asset->getGuid(), asset);
        return asset;
    }

    Asset* AssetManager::getAsset(const std::string& path)
    {
        auto findResult = mPathAssetMap.find(path);
        if (findResult != mPathAssetMap.end())
            return findResult->second;
        return nullptr;
    }

    Asset* AssetManager::getAsset(Guid guid)
    {
        auto findResult = mGuidAssetMap.find(guid);
        if (findResult != mGuidAssetMap.end())
            return findResult->second;
        return nullptr;
    }
}
