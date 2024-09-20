#pragma once
#include "Object.h"
#include "Serialization/AssetLoader.h"
#include "Serialization/AssetSaver.h"

#include <osg/Referenced>
#include <osg/ref_ptr>
#include <string>
#include <fstream>
#include <filesystem>
#include <unordered_set>

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
        uint64_t magic = 0x5858584153534554; // XXXASSET
        uint32_t version = 0;
        uint32_t flags = 0;
        Guid     guid = Guid();
        uint32_t stringTableSize;
        uint32_t importTableSize;
        uint32_t exportTableSize;
        uint32_t objectBufferCount;
    };

    class Asset : public osg::Referenced
    {
    public:
        Asset(const std::string& path) : mPath(path)
        {
            if (std::filesystem::exists(mPath))
            {
                std::ifstream ifs(mPath, std::ios::binary);
                readAssetHeader(ifs);
            }
        }

        virtual ~Asset() = default;

        template <typename T = Object, std::enable_if_t<std::is_base_of_v<Object, T>, int> = 0>
        T* getRootObject()
        {
            return dynamic_cast<T*>(mRootObject.get());
        }

        void setRootObject(Object* rootObject)
        {
            mRootObject = rootObject;
            if (rootObject)
            {
                rootObject->mAsset = this;
                mHeader.guid = rootObject->getGuid();
            }
        }

        std::string& getPath()
        {
            return mPath;
        }

        const std::string& getPath() const
        {
            return mPath;
        }

        Guid getGuid() const
        {
            return mHeader.guid;
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
            AssetSerializer* assetLoader = new AssetLoader(this);

            std::ifstream ifs(mPath, std::ios::binary);
            readAssetHeader(ifs);
            readStringTable(assetLoader, ifs);
            readImportTable(assetLoader, ifs);
            readExportEable(assetLoader, ifs);
            readObjectBuffers(assetLoader, ifs);

            Object* rootObject = nullptr;
            assetLoader->serializeClass(refl::Reflection::getClass(assetLoader->mStringTable[0]), &rootObject);
            setRootObject(rootObject);

            mNeedSave = false;
            mIsLoaded = true;
        }

        void forceSave()
        {
            AssetSerializer* assetSaver = new AssetSaver(this);

            Object* rootObject = getRootObject();
            assetSaver->mStringTable.emplace_back(rootObject->getClass()->getName());
            assetSaver->serializeClass(rootObject->getClass(), &rootObject);

            std::ofstream ofs(mPath, std::ios::binary);
            writeAssetHeader(assetSaver, ofs);
            writeStringTable(assetSaver, ofs);
            writeImportTable(assetSaver, ofs);
            writeExportTable(assetSaver, ofs);
            writeObjectBuffers(assetSaver, ofs);

            mNeedSave = false;
            mIsLoaded = true;
        }

        void removeObject(Object* object)
        {
            //mExportedObjects.erase(object);
        }

    protected:
        std::string mPath;
        AssetHeader mHeader;
        bool mIsLoaded = false;
        bool mNeedSave = false;

        osg::ref_ptr<Object> mRootObject;

        // Asset 中需要保留导入表, 以供删除其它资产时查询本资产是否引用其对象
        std::vector<Asset*> mImportedObjects;
        // Asset 中需要保留导出表;
        std::vector<osg::ref_ptr<Object>> mExportedObjects;

        void readAssetHeader(std::ifstream& ifs)
        {
            ifs.read((char*)&mHeader, sizeof(mHeader));
        }

        void readStringTable(AssetSerializer* serializer, std::ifstream& ifs)
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
                serializer->mStringTable.emplace_back((char*)dataPtr, stringLength);
                dataPtr += stringLength;
            }
        }

        void readImportTable(AssetSerializer* serializer, std::ifstream& ifs)
        {
            std::vector<uint8_t> buffer(mHeader.importTableSize);
            uint8_t* dataPtr = buffer.data();
            ifs.read((char*)(dataPtr), mHeader.importTableSize);

            uint32_t importCount = *(uint32_t*)(dataPtr);
            dataPtr += sizeof(uint32_t);

            for (uint32_t i = 0; i < importCount; ++i)
            {
                Guid guid = *(Guid*)(dataPtr);
                serializer->mImportTable.emplace_back(guid);
                dataPtr += sizeof(Guid);
            }
        }

        void readExportEable(AssetSerializer* serializer, std::ifstream& ifs)
        {
            std::vector<uint8_t> buffer(mHeader.exportTableSize);
            uint8_t* dataPtr = buffer.data();
            ifs.read((char*)(dataPtr), mHeader.exportTableSize);

            uint32_t exportCount = *(uint32_t*)(dataPtr);
            dataPtr += sizeof(uint32_t);

            for (uint32_t i = 0; i < exportCount; ++i)
            {
                serializer->mExportTable.emplace_back(*(Guid*)(dataPtr));
                dataPtr += sizeof(Guid);
            }
        }

        void readObjectBuffers(AssetSerializer* serializer, std::ifstream& ifs)
        {
            for (uint32_t i = 0; i < mHeader.objectBufferCount; ++i)
            {
                uint32_t objectBufferSize = 0;
                ifs.read((char*)(&objectBufferSize), sizeof(uint32_t));
                AssetSerializer::ObjectBuffer& objectBuffer = serializer->mObjectBufferTable.emplace_back(objectBufferSize, 0);
                ifs.read((char*)(objectBuffer.buffer.data()), objectBufferSize);
            }
        }

        void writeAssetHeader(AssetSerializer* serializer, std::ofstream& ofs)
        {
            mHeader.stringTableSize = sizeof(uint32_t);
            {
                for (auto& it : serializer->mStringTable)
                    mHeader.stringTableSize += sizeof(uint32_t) + it.size();
            }
            mHeader.importTableSize = sizeof(uint32_t) + sizeof(Guid) * serializer->mImportTable.size();
            mHeader.exportTableSize = sizeof(uint32_t) + sizeof(Guid) * serializer->mExportTable.size();
            mHeader.objectBufferCount = serializer->mObjectBufferTable.size();
            ofs.write((char*)(&mHeader), sizeof(AssetHeader));
        }

        void writeStringTable(AssetSerializer* serializer, std::ofstream& ofs)
        {
            std::vector<uint8_t> buffer(mHeader.stringTableSize);
            uint8_t* dataPtr = buffer.data();

            *(uint32_t*)(dataPtr) = static_cast<uint32_t>(serializer->mStringTable.size());
            dataPtr += sizeof(uint32_t);

            for (auto& it : serializer->mStringTable)
            {
                *(uint32_t*)(dataPtr) = static_cast<uint32_t>(it.size());
                dataPtr += sizeof(uint32_t);
                std::memcpy(dataPtr, it.data(), it.size());
                dataPtr += it.size();
            }

            ofs.write((const char*)(buffer.data()), mHeader.stringTableSize);
        }

        void writeImportTable(AssetSerializer* serializer, std::ofstream& ofs)
        {
            std::vector<uint8_t> buffer(mHeader.importTableSize);
            uint8_t* dataPtr = buffer.data();

            *(uint32_t*)(dataPtr) = static_cast<uint32_t>(serializer->mImportTable.size());
            dataPtr += sizeof(uint32_t);

            for (auto it : serializer->mImportTable)
            {
                *(Guid*)(dataPtr) = it;
                dataPtr += sizeof(Guid);
            }

            ofs.write((const char*)(buffer.data()), mHeader.importTableSize);
        }

        void writeExportTable(AssetSerializer* serializer, std::ofstream& ofs)
        {
            std::vector<uint8_t> buffer(mHeader.exportTableSize);
            uint8_t* dataPtr = buffer.data();

            *(uint32_t*)(dataPtr) = static_cast<uint32_t>(serializer->mExportTable.size());
            dataPtr += sizeof(uint32_t);

            for (auto it : serializer->mExportTable)
            {
                *(Guid*)(dataPtr) = it;
                dataPtr += sizeof(Guid);
            }

            ofs.write((const char*)(buffer.data()), mHeader.exportTableSize);
        }

        void writeObjectBuffers(AssetSerializer* serializer, std::ofstream& ofs)
        {
            for (uint32_t i = 0; i < mHeader.objectBufferCount; ++i)
            {
                AssetSerializer::ObjectBuffer& objectBuffer = serializer->mObjectBufferTable.at(i);
                uint32_t bufferSize = objectBuffer.buffer.size();
                ofs.write((char*)&bufferSize, sizeof(uint32_t));
                ofs.write((char*)objectBuffer.buffer.data(), bufferSize);
            }
        }
    };
}
