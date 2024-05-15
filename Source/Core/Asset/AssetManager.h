#pragma once
#include "Material.h"
#include "StaticMesh.h"
#include <unordered_map>

namespace xxx
{
    enum class AssetType
    {
        None_Type = -1,
        Texture,
        Material,
        StaticMesh,
        AssetType_Count,
    };

    class AssetManager : osg::Referenced
    {
    public:
        AssetManager() = default;
        virtual ~AssetManager() = default;

        Asset* loadAsset(const fs::path& assetPath);

    private:
        template <typename T>
        using AssetMap = std::unordered_map<std::string_view, osg::ref_ptr<T>>;

        AssetMap<Material> _materialMap;
        AssetMap<StaticMesh> _staticMeshMap;

    };
}
