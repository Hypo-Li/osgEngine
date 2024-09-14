#include "AssetManager.h"

namespace xxx
{
    using namespace refl;

    Asset* AssetManager::createAsset(Object* rootObject, const std::string& assetPath)
    {
        Asset* asset = new Asset(assetPath);
        setObjectAssetRecursively(rootObject, asset);
        return asset;
    }

    Asset* AssetManager::getAsset(const std::string& path)
    {
        auto findResult = mAssets.find(path);
        if (findResult != mAssets.end())
            return findResult->second;
        return nullptr;
    }

    void AssetManager::setObjectAssetRecursively(Object* object, Asset* asset)
    {
        object->mAsset = asset;
        Class* clazz = object->getClass();
        while (clazz)
        {
            for (Property* prop : clazz->getProperties())
            {
                bool isClass = prop->getDeclaredType()->getKind() == Type::Kind::Class;
                Object* propObject;
                prop->getValue(object, &propObject);
                bool hasNoAsset = propObject->getAsset() == nullptr;
                if (isClass && hasNoAsset)
                    setObjectAssetRecursively(propObject, asset);
            }
            clazz = clazz->getBaseClass();
        }
    }
}
