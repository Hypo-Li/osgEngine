#pragma once
#include "Object.h"
#include "Serialization/AssetLoader.h"
#include "Serialization/AssetSaver.h"

#include <osg/Referenced>
#include <osg/ref_ptr>
#include <string>
#include <fstream>
#include <filesystem>

namespace xxx
{
    /**
    * Asset Header
    * String Table
    * Import Table
    * Export Table
    * Object Buffers
    */

    struct AssetHeader
    {
        uint32_t magic;
        uint32_t version;
        uint32_t flags;
        uint32_t headerSize;
        uint32_t stringTableSize;
        uint32_t importTableSize;
        uint32_t exportTableSize;
        
    };

    using ImportTable = std::unordered_map<Asset*, Guid>;
    using ExportTable = std::set<osg::ref_ptr<Object>>;

    class Asset : public osg::Referenced
    {
    public:

        Asset(const std::string& path);
        virtual ~Asset() = default;

        template <typename T = Object, std::enable_if_t<std::is_base_of_v<Object, T>, int> = 0>
        T* getRootObject()
        {
            return dynamic_cast<T*>(mRootObject.get());
        }

        template <typename T = Object, std::enable_if_t<std::is_base_of_v<Object, T>, int> = 0>
        T* findObjectByGuid(Guid guid)
        {
            auto findResult = mObjectTable.find(guid);
            if (findResult != mObjectTable.end())
                return dynamic_cast<T*>(findResult->second.get());
            return nullptr;
        }

        std::string& getPath()
        {
            return mPath;
        }

        const std::string& getPath() const
        {
            return mPath;
        }

        void load()
        {
            if (!mIsLoaded)
                forceLoad();
        }

        void save()
        {
            if (!mNeedSave)
                forceSave();
        }

        void forceLoad()
        {
            //std::filesystem::path assetPath;
            //std::ifstream ifs(assetPath, std::ios::binary);
            //AssetLoader* assetLoader = new AssetLoader(this);
            //readAssetHeader(assetLoader, ifs);
            //readStringTable(assetLoader, ifs);
            //readImportTable(assetLoader, ifs);
            //readExportEable(assetLoader, ifs);
            mIsLoaded = true;

        }

        void forceSave()
        {
            //std::filesystem::path assetPath;
            //std::ofstream ofs(assetPath, std::ios::binary);
            //AssetSaver* assetSaver = new AssetSaver;
            //writeAssetHeader(assetSaver, ofs);
            //writeStringTable(assetSaver, ofs);
            //writeImportTable(assetSaver, ofs);
            //writeExportTable(assetSaver, ofs);
            mNeedSave = false;
        }

        void setRootObject(Object* rootObject)
        {
            mRootObject = rootObject;
        }

        void removeObject(Object* object)
        {
            mExportTable.erase(object);
        }

    protected:
        std::string mPath;
        bool mIsLoaded = false;
        bool mNeedSave = false;
        uint32_t mFlags;
        osg::ref_ptr<Object> mRootObject;

        // Asset 中需要保留导入表, 以供删除其它资产时查询本资产是否引用其对象
        ImportTable mImportTable;
        // Asset 中需要保留导出表;
        ExportTable mExportTable;

        //void readAssetHeader(AssetSerializer* serializer, std::ifstream& ifs)
        //{
        //    ifs.read((char*)(&serializer->mAssetHeader), sizeof(AssetHeader));
        //}

        //void readStringTable(AssetSerializer* serializer, std::ifstream& ifs)
        //{
        //    std::vector<uint8_t> buffer(serializer->mAssetHeader.stringTableSize);
        //    uint8_t* dataPtr = buffer.data();
        //    ifs.read((char*)(dataPtr), serializer->mAssetHeader.stringTableSize);

        //    uint32_t stringCount = *(uint32_t*)(dataPtr);
        //    dataPtr += sizeof(uint32_t);
        //    for (uint32_t i = 0; i < stringCount; ++i)
        //    {
        //        uint32_t stringLength = *(uint32_t*)(dataPtr);
        //        dataPtr += sizeof(uint32_t);
        //        serializer->mStringTable.emplace_back((char*)dataPtr, stringLength);
        //        dataPtr += stringLength;
        //    }
        //}

        //void readImportTable(AssetSerializer* serializer, std::ifstream& ifs)
        //{
        //    std::vector<uint8_t> buffer(mHeader.importTableSize);
        //    uint8_t* dataPtr = buffer.data();
        //    ifs.read((char*)(dataPtr), mHeader.importTableSize);

        //    uint32_t importCount = *(uint32_t*)(dataPtr);
        //    dataPtr += sizeof(uint32_t);
        //    for (uint32_t i = 0; i < importCount; ++i)
        //    {
        //        uint32_t pathStringIndex = *(uint32_t*)(dataPtr);
        //        dataPtr += sizeof(uint32_t);
        //        mImportTable.emplace_back(pathStringIndex, *(Guid*)(dataPtr));
        //        dataPtr += sizeof(Guid);
        //    }
        //}

        //void readExportEable(AssetSerializer* serializer, std::ifstream& ifs)
        //{
        //    std::vector<uint8_t> buffer(mHeader.exportTableSize);
        //    uint8_t* dataPtr = buffer.data();
        //    ifs.read((char*)(dataPtr), mHeader.exportTableSize);

        //    uint32_t exportCount = *(uint32_t*)(dataPtr);
        //    dataPtr += sizeof(uint32_t);
        //    for (uint32_t i = 0; i < exportCount; ++i)
        //    {
        //        mExportTable.emplace_back(*(Guid*)(dataPtr));
        //        dataPtr += sizeof(Guid);
        //    }
        //}

        //void writeAssetHeader(AssetSerializer* serializer, std::ofstream& ofs)
        //{
        //    mHeader.magic = 0xA55E7D06; // Asset Dog
        //    mHeader.headerSize = sizeof(AssetHeader);
        //    mHeader.stringTableSize = sizeof(uint32_t);
        //    {
        //        for (auto& it : mStringTable)
        //            mHeader.stringTableSize += sizeof(uint32_t) + it.size();
        //    }
        //    mHeader.importTableSize = sizeof(uint32_t) + (sizeof(uint32_t) + sizeof(Guid)) * mImportTable.size();
        //    mHeader.exportTableSize = sizeof(uint32_t) + sizeof(Guid) * mExportTable.size();

        //    ofs.write((char*)(&mHeader), sizeof(AssetHeader));
        //}

        //void writeStringTable(AssetSerializer* serializer, std::ofstream& ofs)
        //{
        //    std::vector<uint8_t> buffer(mHeader.stringTableSize);
        //    uint8_t* dataPtr = buffer.data();

        //    *(uint32_t*)(dataPtr) = static_cast<uint32_t>(mStringTable.size());
        //    dataPtr += sizeof(uint32_t);
        //    for (auto& it : mStringTable)
        //    {
        //        *(uint32_t*)(dataPtr) = static_cast<uint32_t>(it.size());
        //        dataPtr += sizeof(uint32_t);
        //        std::memcpy(dataPtr, it.data(), it.size());
        //        dataPtr += it.size();
        //    }
        //    ofs.write((const char*)(buffer.data()), mHeader.stringTableSize);
        //}

        //void writeImportTable(AssetSerializer* serializer, std::ofstream& ofs)
        //{
        //    std::vector<uint8_t> buffer(mHeader.importTableSize);
        //    uint8_t* dataPtr = buffer.data();

        //    *(uint32_t*)(dataPtr) = static_cast<uint32_t>(mImportTable.size());
        //    dataPtr += sizeof(uint32_t);
        //    for (auto& it : mImportTable)
        //    {
        //        *(uint32_t*)(dataPtr) = it.first;
        //        dataPtr += sizeof(uint32_t);
        //        *(Guid*)(dataPtr) = it.second;
        //        dataPtr += sizeof(Guid);
        //    }
        //    ofs.write((const char*)(buffer.data()), mHeader.importTableSize);
        //}

        //void writeExportTable(AssetSerializer* serializer, std::ofstream& ofs)
        //{
        //    std::vector<uint8_t> buffer(mHeader.exportTableSize);
        //    uint8_t* dataPtr = buffer.data();

        //    *(uint32_t*)(dataPtr) = static_cast<uint32_t>(mExportTable.size());
        //    dataPtr += sizeof(uint32_t);
        //    for (auto it : mExportTable)
        //    {
        //        *(Guid*)(dataPtr) = it;
        //        dataPtr += sizeof(Guid);
        //    }
        //    ofs.write((const char*)(buffer.data()), mHeader.exportTableSize);
        //}
    };
}
