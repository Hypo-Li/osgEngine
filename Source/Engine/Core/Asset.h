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

    class AssetSerializer;
    class AssetLoader;
    class AssetSaver;
    class Asset : public osg::Referenced
    {
        friend class AssetSerializer;
        friend class AssetLoader;
        friend class AssetSaver;
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
            std::filesystem::path assetPath;
            std::ifstream ifs(assetPath, std::ios::binary);
            readAssetHeader(ifs);
            readStringTable(ifs);
            readImportTable(ifs);
            readExportEable(ifs);
            AssetLoader* assetLoader = new AssetLoader(this);

        }

        void forceSave()
        {
            std::filesystem::path assetPath;
            std::ofstream ofs(assetPath, std::ios::binary);
            writeAssetHeader(ofs);
            writeStringTable(ofs);
            writeImportTable(ofs);
            writeExportTable(ofs);
            AssetSaver* assetSaver = new AssetSaver;
        }

        void setRootObject(Object* rootObject)
        {
            mRootObject = rootObject;
        }

    protected:
        std::string mPath;
        bool mIsLoaded = false;
        bool mNeedSave = false;

        AssetHeader mHeader;
        std::vector<std::string> mStringTable;
        std::vector<std::pair<uint32_t, Guid>> mImportTable;
        std::vector<Guid> mExportTable;

        osg::ref_ptr<Object> mRootObject;
        std::unordered_map<Guid, osg::ref_ptr<Object>> mObjectTable;

        void readAssetHeader(std::ifstream& ifs)
        {
            ifs.read((char*)(&mHeader), sizeof(AssetHeader));
        }

        void readStringTable(std::ifstream& ifs)
        {
            std::vector<uint8_t> buffer(mHeader.stringTableSize);
            uint8_t* dataPtr = buffer.data();
            ifs.read((char*)(dataPtr), mHeader.stringTableSize);

            uint32_t stringCount = *(uint32_t*)(dataPtr);
            dataPtr += sizeof(uint32_t);
            for (uint32_t i = 0; i < stringCount; ++i)
            {
                uint32_t stringLength = *(uint32_t*)(dataPtr);
                dataPtr += sizeof(uint32_t);
                mStringTable.emplace_back((char*)dataPtr, stringLength);
                dataPtr += stringLength;
            }
        }

        void readImportTable(std::ifstream& ifs)
        {
            std::vector<uint8_t> buffer(mHeader.importTableSize);
            uint8_t* dataPtr = buffer.data();
            ifs.read((char*)(dataPtr), mHeader.importTableSize);

            uint32_t importCount = *(uint32_t*)(dataPtr);
            dataPtr += sizeof(uint32_t);
            for (uint32_t i = 0; i < importCount; ++i)
            {
                uint32_t pathStringIndex = *(uint32_t*)(dataPtr);
                dataPtr += sizeof(uint32_t);
                mImportTable.emplace_back(pathStringIndex, *(Guid*)(dataPtr));
                dataPtr += sizeof(Guid);
            }
        }

        void readExportEable(std::ifstream& ifs)
        {
            std::vector<uint8_t> buffer(mHeader.exportTableSize);
            uint8_t* dataPtr = buffer.data();
            ifs.read((char*)(dataPtr), mHeader.exportTableSize);

            uint32_t exportCount = *(uint32_t*)(dataPtr);
            dataPtr += sizeof(uint32_t);
            for (uint32_t i = 0; i < exportCount; ++i)
            {
                mExportTable.emplace_back(*(Guid*)(dataPtr));
                dataPtr += sizeof(Guid);
            }
        }

        void writeAssetHeader(std::ofstream& ofs)
        {
            mHeader.magic = 0xA55E7D06; // Asset Dog
            mHeader.headerSize = sizeof(AssetHeader);
            mHeader.stringTableSize = sizeof(uint32_t);
            {
                for (auto& it : mStringTable)
                    mHeader.stringTableSize += sizeof(uint32_t) + it.size();
            }
            mHeader.importTableSize = sizeof(uint32_t) + (sizeof(uint32_t) + sizeof(Guid)) * mImportTable.size();
            mHeader.exportTableSize = sizeof(uint32_t) + sizeof(Guid) * mExportTable.size();

            ofs.write((char*)(&mHeader), sizeof(AssetHeader));
        }

        void writeStringTable(std::ofstream& ofs)
        {
            std::vector<uint8_t> buffer(mHeader.stringTableSize);
            uint8_t* dataPtr = buffer.data();

            *(uint32_t*)(dataPtr) = static_cast<uint32_t>(mStringTable.size());
            dataPtr += sizeof(uint32_t);
            for (auto& it : mStringTable)
            {
                *(uint32_t*)(dataPtr) = static_cast<uint32_t>(it.size());
                dataPtr += sizeof(uint32_t);
                std::memcpy(dataPtr, it.data(), it.size());
                dataPtr += it.size();
            }
            ofs.write((const char*)(buffer.data()), mHeader.stringTableSize);
        }

        void writeImportTable(std::ofstream& ofs)
        {
            std::vector<uint8_t> buffer(mHeader.importTableSize);
            uint8_t* dataPtr = buffer.data();

            *(uint32_t*)(dataPtr) = static_cast<uint32_t>(mImportTable.size());
            dataPtr += sizeof(uint32_t);
            for (auto& it : mImportTable)
            {
                *(uint32_t*)(dataPtr) = it.first;
                dataPtr += sizeof(uint32_t);
                *(Guid*)(dataPtr) = it.second;
                dataPtr += sizeof(Guid);
            }
            ofs.write((const char*)(buffer.data()), mHeader.importTableSize);
        }

        void writeExportTable(std::ofstream& ofs)
        {
            std::vector<uint8_t> buffer(mHeader.exportTableSize);
            uint8_t* dataPtr = buffer.data();

            *(uint32_t*)(dataPtr) = static_cast<uint32_t>(mExportTable.size());
            dataPtr += sizeof(uint32_t);
            for (auto it : mExportTable)
            {
                *(Guid*)(dataPtr) = it;
                dataPtr += sizeof(Guid);
            }
            ofs.write((const char*)(buffer.data()), mHeader.exportTableSize);
        }
    };

    class AssetRoot
    {
    public:
        void setAsset(Asset* asset)
        {
            mAsset = asset;
        }

        Asset* getAsset() const
        {
            return mAsset;
        }

    protected:
        Asset* mAsset;
    };
}
