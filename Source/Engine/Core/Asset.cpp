#include "Asset.h"
#include "AssetManager.h"

namespace xxx
{
    using namespace refl;

    Asset::Asset(const std::string& path) : mPath(path)
    {
        if (std::filesystem::exists(mPath))
        {
            std::ifstream ifs(mPath, std::ios::binary);
            AssetHeader header;
            ifs.read((char*)&header, sizeof(AssetHeader));
            {
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
            }

            mGuid = header.guid;
        }
    }

    void Asset::setRootObject(Object* rootObject)
    {
        mRootObject = rootObject;
        if (rootObject)
        {
            Guid newGuid = rootObject->getGuid();
            if (mGuid != newGuid)
            {
                AssetManager& am = AssetManager::get();
                am.mGuidAssetMap.erase(mGuid);
                am.mGuidAssetMap.emplace(newGuid, this);
                mGuid = newGuid;
            }
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
        if (mIsLoaded)
            return;

        std::ifstream ifs(mPath, std::ios::binary);
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
        std::ofstream ofs(mPath, std::ios::binary);
        AssetSerializer* assetSaver = new AssetSaver(this);
        AssetHeader header;
        header.guid = mGuid;

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
}
