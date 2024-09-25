#include "AssetManager.h"

namespace xxx
{
    using namespace refl;

    Asset* AssetManager::createAsset(Object* rootObject, const std::string& path)
    {
        std::filesystem::path fsPath(path);
        std::string pathStr = fsPath.make_preferred().string();
        auto findResult = mPathAssetMap.find(pathStr);
        if (findResult != mPathAssetMap.end())
        {
            return nullptr;
            /*Asset* asset = findResult->second;
            mPathAssetMap.erase(findResult);
            mGuidAssetMap.erase(asset->getGuid());
            mAssets.erase(asset);*/
        }

        Asset* asset = new Asset(pathStr);
        asset->setRootObject(rootObject);
        mAssets.emplace(asset);
        mPathAssetMap.emplace(pathStr, asset);
        mGuidAssetMap.emplace(asset->getGuid(), asset);
        return asset;
    }

    Asset* AssetManager::getAsset(const std::string& path)
    {
        std::filesystem::path fsPath(path);
        std::string pathStr = fsPath.make_preferred().string();
        auto findResult = mPathAssetMap.find(pathStr);
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
