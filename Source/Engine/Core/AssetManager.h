#pragma once
#include "Asset.h"
#include "Context.h"

#include <iostream>

namespace xxx
{
    class AssetManager
    {
        friend class Asset;
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

        template <typename T = void>
        void foreachAsset(const std::function<void(Asset*)>& func)
        {
            refl::Class* clazz = refl::Reflection::getClass<T>();
            for (Asset* asset : mAssets)
            {
                if constexpr (!std::is_same_v<T, void>)
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

    private:
        std::unordered_set<osg::ref_ptr<Asset>> mAssets;
        std::unordered_map<std::string, Asset*> mPathAssetMap;
        std::unordered_map<Guid, Asset*> mGuidAssetMap;
    };
}
