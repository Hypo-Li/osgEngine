#include "Asset.h"
#include "AssetManager.h"

namespace xxx
{
    using namespace refl;

    Asset::Asset(const std::string& path) :
        mPath(path),
        mName(mPath.substr(mPath.find_last_of('/') + 1))
    {
        std::filesystem::path fullPath = convertAssetPathToFullPath(mPath);

        if (std::filesystem::exists(fullPath))
        {
            std::ifstream ifs(fullPath, std::ios::binary);
            AssetHeader header;
            ifs.read((char*)&header, sizeof(AssetHeader));
            {
                ifs.seekg(sizeof(AssetHeader) + sizeof(uint32_t));
                uint32_t classNameLength;
                ifs.read((char*)(&classNameLength), sizeof(classNameLength));
                std::string className(classNameLength, ' ');
                ifs.read(className.data(), classNameLength);
                mClass = Reflection::getClass(className);

                // load import table
                ifs.seekg(sizeof(AssetHeader) + header.stringTableSize);
                std::vector<uint8_t> buffer(header.importTableSize);
                uint8_t* dataPtr = buffer.data();
                ifs.read((char*)(dataPtr), header.importTableSize);

                uint32_t importCount = *(uint32_t*)(dataPtr);
                dataPtr += sizeof(uint32_t);

                for (uint32_t i = 0; i < importCount; ++i)
                {
                    ImportItem importItem = *(ImportItem*)(dataPtr);
                    dataPtr += sizeof(ImportItem);
                    mImportedObjects.insert(importItem.objectGuid);
                }

                ifs.seekg(sizeof(AssetHeader) + header.stringTableSize + header.importTableSize + sizeof(uint32_t));
                ExportItem rootObjectItem;
                ifs.read((char*)(&rootObjectItem), sizeof(ExportItem));
                mGuid = rootObjectItem.objectGuid;
            }
        }
    }

    void Asset::setRootObject(Object* rootObject)
    {
        if (mRootObject == rootObject)
            return;

        AssetManager& am = AssetManager::get();
        am.mGuidAssetMap.erase(mGuid);
        mRootObject = rootObject;
        if (rootObject)
        {
            Guid newGuid = rootObject->getGuid();
            mGuid = newGuid;
            am.mGuidAssetMap.emplace(newGuid, this);
            mClass = mRootObject->getClass();
            mIsLoaded = true;
        }
        else if (mGuid.isValid())
        {
            am.mGuidAssetMap.emplace(mGuid, this);
        }
    }

    void Asset::setPath(const std::string& path)
    {
        std::string newPath = std::filesystem::path(path).make_preferred().string();
        if (mPath == newPath)
        {
            AssetManager& am = AssetManager::get();
            am.mPathAssetMap.erase(mPath);
            am.mPathAssetMap.emplace(newPath, this);
            mPath = newPath;
        }
    }

    void Asset::load()
    {
        std::ifstream ifs(convertAssetPathToFullPath(mPath), std::ios::binary);
        AssetSerializer* assetLoader = new AssetLoader(this);
        AssetHeader header;

        readAssetHeader(ifs, header);
        readStringTable(ifs, assetLoader, header);
        readImportTable(ifs, assetLoader, header);
        readExportEable(ifs, assetLoader, header);
        readObjectBuffers(ifs, assetLoader, header);

        mImportedObjects.clear();
        std::unordered_set<osg::ref_ptr<Object>> exportedObjectsTemp(mExportedObjects.begin(), mExportedObjects.end());
        mExportedObjects.clear();

        Object* rootObject = nullptr;
        assetLoader->serialize(&rootObject);
        setRootObject(rootObject);

        mIsLoaded = true;
    }

    void Asset::save()
    {
        std::ofstream ofs(convertAssetPathToFullPath(mPath), std::ios::binary);
        AssetSerializer* assetSaver = new AssetSaver(this);
        AssetHeader header;

        mImportedObjects.clear();
        std::unordered_set<osg::ref_ptr<Object>> exportedObjectsTemp(mExportedObjects.begin(), mExportedObjects.end());
        mExportedObjects.clear();

        Object* rootObject = getRootObject();
        assetSaver->serialize(&rootObject);

        writeAssetHeader(ofs, assetSaver, header);
        writeStringTable(ofs, assetSaver, header);
        writeImportTable(ofs, assetSaver, header);
        writeExportTable(ofs, assetSaver, header);
        writeObjectBuffers(ofs, assetSaver, header);

        mIsLoaded = true;
    }

    std::string Asset::convertFullPathToAssetPath(const std::filesystem::path& fullPath)
    {
        if (fullPath.extension() != ".xast")
            return std::string();

        const std::filesystem::path& engineAssetPath = Context::get().getEngineAssetPath();
        if (fullPath.compare(engineAssetPath) > 0)
        {
            std::filesystem::path relativePath = fullPath.lexically_relative(engineAssetPath);
            relativePath.replace_extension("");
            std::string assetPath = "Engine/" + relativePath.string();
            std::replace(assetPath.begin(), assetPath.end(), '\\', '/');
            return assetPath;
        }

        const std::filesystem::path& projectAssetPath = Context::get().getProjectAssetPath();
        if (fullPath.compare(projectAssetPath) > 0)
        {
            std::filesystem::path relativePath = fullPath.lexically_relative(projectAssetPath);
            relativePath.replace_extension("");
            std::string assetPath = "Project/" + relativePath.string();
            std::replace(assetPath.begin(), assetPath.end(), '\\', '/');
            return assetPath;
        }

        return std::string();
    }

    std::filesystem::path Asset::convertAssetPathToFullPath(const std::string& assetPath)
    {
        std::filesystem::path fullPath;
        if (assetPath.substr(0, 7) == "Engine/")
        {
            fullPath = Context::get().getEngineAssetPath();
            fullPath /= assetPath.substr(7);
            fullPath += ".xast";
            return fullPath;
        }

        if (assetPath.substr(0, 1) == "Project/")
        {
            fullPath = Context::get().getProjectAssetPath();
            fullPath /= assetPath.substr(8);
            fullPath += ".xast";
            return fullPath;
        }

        // LogError illegal path
        return std::filesystem::path();
    }
}
