#include "AssetManager.h"

namespace xxx
{
    using namespace refl;

    Asset* AssetManager::createAsset(const std::string& path, Object* rootObject)
    {
        auto findResult = mPathAssetMap.find(path);
        Asset* asset = nullptr;
        if (findResult == mPathAssetMap.end())
        {
            asset = new Asset(path, rootObject);
            mAssets.insert(asset);
            mPathAssetMap.emplace(path, asset);
            if (asset->getGuid().isValid())
                mGuidAssetMap.emplace(asset->getGuid(), asset);
        }
        else
        {
            asset = findResult->second;
            if (asset->getState() == Asset::State::Unloaded)
                setAssetRootObject(asset, rootObject);
        }
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
