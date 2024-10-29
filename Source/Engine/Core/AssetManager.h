#pragma once
#include "Asset.h"
#include "Context.h"

#include <iostream>

namespace xxx
{
    class AssetManager
    {
    public:
        AssetManager()
        {
            const std::filesystem::path& engineAssetPath = Context::get().getEngineAssetPath();
            for (const auto& file : std::filesystem::recursive_directory_iterator(engineAssetPath))
            {
                if (file.is_regular_file())
                {
                    createAsset(nullptr, Asset::convertFullPathToAssetPath(file.path()));
                }
            }

            /*const std::filesystem::path& projectAssetPath = Context::get().getProjectAssetPath();
            for (const auto& file : std::filesystem::recursive_directory_iterator(projectAssetPath))
            {
                if (file.is_regular_file())
                {
                    std::filesystem::path fullPath = file.path();
                    std::filesystem::path relativePath = fullPath.lexically_relative(projectAssetPath);

                    createAsset(nullptr, "/" + relativePath.string());
                }
            }*/

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

        template <typename Filter = void>
        void foreachAsset(const std::function<void(Asset*)>& func)
        {
            refl::Class* clazz = refl::Reflection::getClass<Filter>();
            for (Asset* asset : mAssets)
            {
                if constexpr (!std::is_same_v<Filter, void>)
                {
                    if (clazz == asset->getClass())
                        func(asset);
                }
                else
                {
                    func(asset);
                }
            }
        }

        void setAssetRootObject(Asset* asset, Object* rootObject)
        {
            if (asset->mRootObject == rootObject)
                return;

            if (rootObject && rootObject->getGuid() != asset->getGuid())
            {
                mGuidAssetMap.erase(asset->getGuid());
                mGuidAssetMap.emplace(rootObject->getGuid(), asset);
            }

            asset->setRootObject(rootObject);
        }

        void setAssetPath(Asset* asset, const std::string& path)
        {
            if (asset->mPath == path)
                return;

            mPathAssetMap.erase(asset->mPath);
            mPathAssetMap.emplace(path, asset);
            asset->setPath(path);
        }

    private:
        std::set<osg::ref_ptr<Asset>> mAssets;
        std::unordered_map<std::string, Asset*> mPathAssetMap;
        std::unordered_map<Guid, Asset*> mGuidAssetMap;
    };
}
