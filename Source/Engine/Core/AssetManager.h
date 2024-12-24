#pragma once
#include "Asset.h"
#include "Context.h"

#include <iostream>
#include <functional>

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
                if (file.is_regular_file() && file.path().extension() == Asset::sAssetExtension)
                {
                    createAsset(Asset::convertPhysicalPathToAssetPath(file.path()));
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
        }

        static AssetManager& get()
        {
            static AssetManager assetManager;
            return assetManager;
        }

        Asset* createAsset(const std::string& path, Object* rootObject = nullptr);

        Asset* getAsset(const std::string& path);

        Asset* getAsset(Guid guid);

        template <typename Filter = void>
        void foreachAsset(const std::function<void(Asset*)>& func)
        {
            refl::Class* clazz = refl::Reflection::getClass<Filter>();
            for (auto& pair : mPathAssetMap)
            {
                Asset* asset = pair.second;
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
            if (asset->getRootObject() == rootObject)
                return;

            if (asset->getGuid().isValid())
                mGuidAssetMap.erase(asset->getGuid());

            if (rootObject)
                mGuidAssetMap.emplace(rootObject->getGuid(), asset);

            asset->setRootObject(rootObject);

            // TODO: update Referenced Assets
        }

        void setAssetPath(Asset* asset, const std::string& path)
        {
            if (asset->mPath == path)
                return;

            mPathAssetMap.erase(asset->getPath());
            mPathAssetMap.emplace(path, asset);

            asset->setPath(path);

            // TODO: update Referenced Assets
        }

    private:
        std::set<osg::ref_ptr<Asset>> mAssets;
        std::map<std::string, Asset*> mPathAssetMap;
        std::unordered_map<Guid, Asset*> mGuidAssetMap;
    };
}
