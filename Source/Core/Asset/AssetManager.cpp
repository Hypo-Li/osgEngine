#include "AssetManager.h"
#include "MeshAsset.h"
#include <ThirdParty/nlohmann/json.hpp>

namespace xxx
{
    std::unordered_map<std::string, osg::ref_ptr<Asset>> AssetManager::_sAssetMap;

    Asset* AssetManager::loadAsset(const std::string& path)
    {
        Asset* asset = tryGetAssetFromCache(path);
        if (asset)
            return asset;

        std::ifstream ifs(path, std::ios::binary);
        if (!ifs.is_open())
        {
            std::cerr << "failed to open file: " << path << std::endl;
            return nullptr;
        }

        uint32_t assetMagic, typeMagic;
        ifs.read((char*)&assetMagic, sizeof(uint32_t));
        ifs.read((char*)&typeMagic, sizeof(uint32_t));

        if (assetMagic != _sAssetMagic)
        {
            std::cerr << "bad magic: " << path << std::endl;
            return nullptr;
        }

        // NOTE: modify here when add a new asset type
        switch (static_cast<Asset::Type>(typeMagic))
        {
        case Asset::Type::Texture:
            asset = new TextureAsset;
            break;
        case Asset::Type::MaterialTemplate:
            asset = new MaterialTemplateAsset;
            break;
        case Asset::Type::MaterialInstance:
            asset = new MaterialInstanceAsset;
            break;
        case Asset::Type::StaticMesh:
            asset = new StaticMeshAsset;
            break;
        default:
            std::cerr << "unknow asset type: " << path << std::endl;
            return nullptr;
        }
        asset->_path = path;

        uint64_t jsonStrSize, binarySize;
        ifs.read((char*)&jsonStrSize, sizeof(uint64_t));
        ifs.read((char*)&binarySize, sizeof(uint64_t));

        std::string jsonStr(jsonStrSize, 0);
        std::vector<char> binary(binarySize);
        ifs.read(jsonStr.data(), jsonStrSize);
        ifs.read(binary.data(), binarySize);
        ifs.close();

        Json root = Json::parse(jsonStr);
        std::vector<std::string> reference;
        if (root.contains("Reference"))
            reference = root["Reference"];
        Json json;
        if (root.contains("Asset"))
            json = root["Asset"];

        asset->deserialize(json, binary, reference);
        _sAssetMap.insert(std::make_pair(path, asset));
        return asset;
    }

    void AssetManager::storeAsset(const std::string& path, Asset* asset)
    {
        std::ofstream ofs(path, std::ios::binary);
        if (!ofs.is_open())
        {
            std::cerr << "failed to open file: " << path << std::endl;
            return;
        }

        Json json;
        std::vector<char> binary;
        std::vector<std::string> reference;
        asset->serialize(json, binary, reference);

        Json root;
        root["Reference"] = reference;
        root["Asset"] = json;
        std::string jsonStr = root.dump();

        uint32_t assetMagic = _sAssetMagic;
        ofs.write((char*)&assetMagic, sizeof(uint32_t));
        uint32_t typeMagic = static_cast<uint32_t>(asset->getType());
        ofs.write((char*)&typeMagic, sizeof(uint32_t));
        uint64_t jsonStrSize = jsonStr.size();
        uint64_t binarySize = binary.size();
        ofs.write((char*)&jsonStrSize, sizeof(uint64_t));
        ofs.write((char*)&binarySize, sizeof(uint64_t));
        if (jsonStrSize) ofs.write(jsonStr.data(), jsonStrSize);
        if (binarySize) ofs.write(binary.data(), binarySize);
        ofs.close();

        if (asset->_path.size() == 0)
        {
            asset->_path = path;
            _sAssetMap.insert(std::make_pair(path, asset));
        }
    }

    Asset::Type AssetManager::getAssetType(const std::string& path)
    {
        Asset* asset = tryGetAssetFromCache(path);
        if (asset)
            return asset->getType();

        std::ifstream ifs(path, std::ios::binary);
        if (!ifs.is_open())
        {
            std::cerr << "failed to open file: " << path << std::endl;
            return Asset::Type::Unknow;
        }

        uint32_t assetMagic, typeMagic;
        ifs.read((char*)&assetMagic, sizeof(uint32_t));
        ifs.read((char*)&typeMagic, sizeof(uint32_t));
        ifs.close();

        if (assetMagic != _sAssetMagic)
        {
            std::cerr << "bad magic: " << path << std::endl;
            return Asset::Type::Unknow;
        }

        return static_cast<Asset::Type>(typeMagic);
    }

    Asset* AssetManager::tryGetAssetFromCache(const std::string& path)
    {
        const auto& result = _sAssetMap.find(path);
        if (result != _sAssetMap.end())
            return result->second;
        return nullptr;
    }
}
