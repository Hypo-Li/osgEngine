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
        uint64_t magic;
        uint32_t version;
        uint32_t flags;
        uint32_t headerSize;
        uint32_t stringTableSize;
        uint32_t importTableSize;
        uint32_t exportTableSize;
        uint32_t objectBufferCount;
    };

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
            std::filesystem::path assetPath = mPath;
            std::ifstream ifs(assetPath, std::ios::binary);
            AssetSerializer* assetLoader = new AssetLoader(this);

            AssetHeader header;
            readAssetHeader(header, assetLoader, ifs);
            readStringTable(header, assetLoader, ifs);
            readImportTable(header, assetLoader, ifs);
            readExportEable(header, assetLoader, ifs);
            readObjectBuffers(header, assetLoader, ifs);
            mFlags = header.flags;

            Object* rootObject = nullptr;
            assetLoader->serializeClass(refl::Reflection::getClass(assetLoader->mStringTable[0]), &rootObject);
            setRootObject(rootObject);

            mIsLoaded = true;

        }

        void forceSave()
        {
            std::filesystem::path assetPath = mPath;
            std::ofstream ofs(assetPath, std::ios::binary);
            AssetSerializer* assetSaver = new AssetSaver(this);

            Object* rootObject = getRootObject();
            assetSaver->mStringTable.emplace_back(rootObject->getClass()->getName());
            assetSaver->serializeClass(rootObject->getClass(), &rootObject);

            AssetHeader header;
            header.flags = mFlags;
            writeAssetHeader(header, assetSaver, ofs);
            writeStringTable(header, assetSaver, ofs);
            writeImportTable(header, assetSaver, ofs);
            writeExportTable(header, assetSaver, ofs);
            writeObjectBuffers(header, assetSaver, ofs);

            mNeedSave = false;
        }

        void setRootObject(Object* rootObject)
        {
            mRootObject = rootObject;
            setObjectAssetRecursively(rootObject);
        }

        void removeObject(Object* object)
        {
            mExportedObjects.erase(object);
        }

    protected:
        std::string mPath;
        bool mIsLoaded = false;
        bool mNeedSave = false;
        uint32_t mFlags = 0;
        osg::ref_ptr<Object> mRootObject;

        // Asset 中需要保留导入表, 以供删除其它资产时查询本资产是否引用其对象
        std::unordered_set<Asset*> mImportedObjects;
        // Asset 中需要保留导出表;
        std::unordered_set<osg::ref_ptr<Object>> mExportedObjects;

        void setObjectAssetRecursively(Object* object);

        void recordObjects(AssetSerializer* serializer);

        static void readAssetHeader(AssetHeader& header, AssetSerializer* serializer, std::ifstream& ifs)
        {
            ifs.read((char*)(&header), sizeof(AssetHeader));
        }

        static void readStringTable(AssetHeader& header, AssetSerializer* serializer, std::ifstream& ifs)
        {
            std::vector<uint8_t> buffer(header.stringTableSize);
            uint8_t* dataPtr = buffer.data();
            ifs.read((char*)(dataPtr), header.stringTableSize);

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

        static void readImportTable(AssetHeader& header, AssetSerializer* serializer, std::ifstream& ifs)
        {
            std::vector<uint8_t> buffer(header.importTableSize);
            uint8_t* dataPtr = buffer.data();
            ifs.read((char*)(dataPtr), header.importTableSize);

            uint32_t importCount = *(uint32_t*)(dataPtr);
            dataPtr += sizeof(uint32_t);
            for (uint32_t i = 0; i < importCount; ++i)
            {
                uint32_t pathStringIndex = *(uint32_t*)(dataPtr);
                serializer->mImportTable.emplace_back(pathStringIndex);
                dataPtr += sizeof(uint32_t);
            }
        }

        static void readExportEable(AssetHeader& header, AssetSerializer* serializer, std::ifstream& ifs)
        {
            std::vector<uint8_t> buffer(header.exportTableSize);
            uint8_t* dataPtr = buffer.data();
            ifs.read((char*)(dataPtr), header.exportTableSize);

            uint32_t exportCount = *(uint32_t*)(dataPtr);
            dataPtr += sizeof(uint32_t);
            for (uint32_t i = 0; i < exportCount; ++i)
            {
                serializer->mExportTable.emplace_back(*(Guid*)(dataPtr));
                dataPtr += sizeof(Guid);
            }
        }

        static void readObjectBuffers(AssetHeader& header, AssetSerializer* serializer, std::ifstream& ifs)
        {
            for (uint32_t i = 0; i < header.objectBufferCount; ++i)
            {
                uint32_t objectBufferSize = 0;
                ifs.read((char*)(&objectBufferSize), sizeof(uint32_t));
                AssetSerializer::ObjectBuffer& objectBuffer = serializer->mObjectBufferTable.emplace_back(objectBufferSize, 0);
                ifs.read((char*)(objectBuffer.buffer.data()), objectBufferSize);
            }
        }

        static void writeAssetHeader(AssetHeader& header, AssetSerializer* serializer, std::ofstream& ofs)
        {
            char magicChars[8] = { 'X', 'X', 'X', 'A', 'S', 'S', 'E', 'T' };
            header.magic = *(uint64_t*)(magicChars);
            header.version = 0;
            header.headerSize = sizeof(AssetHeader);
            header.stringTableSize = sizeof(uint32_t); // string count
            {
                for (auto& it : serializer->mStringTable)
                    header.stringTableSize += sizeof(uint32_t) + it.size(); // string length + string
            }
            header.importTableSize = sizeof(uint32_t) + sizeof(uint32_t) * serializer->mImportTable.size(); // imported object count + path string indices
            header.exportTableSize = sizeof(uint32_t) + sizeof(Guid) * serializer->mExportTable.size(); // exported object count + object guid
            header.objectBufferCount = serializer->mObjectBufferTable.size();
            ofs.write((char*)(&header), sizeof(AssetHeader));
        }

        static void writeStringTable(AssetHeader& header, AssetSerializer* serializer, std::ofstream& ofs)
        {
            std::vector<uint8_t> buffer(header.stringTableSize);
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
            ofs.write((const char*)(buffer.data()), header.stringTableSize);
        }

        static void writeImportTable(AssetHeader& header, AssetSerializer* serializer, std::ofstream& ofs)
        {
            std::vector<uint8_t> buffer(header.importTableSize);
            uint8_t* dataPtr = buffer.data();

            *(uint32_t*)(dataPtr) = static_cast<uint32_t>(serializer->mImportTable.size());
            dataPtr += sizeof(uint32_t);
            for (auto it : serializer->mImportTable)
            {
                *(uint32_t*)(dataPtr) = it;
                dataPtr += sizeof(uint32_t);
            }
            ofs.write((const char*)(buffer.data()), header.importTableSize);
        }

        static void writeExportTable(AssetHeader& header, AssetSerializer* serializer, std::ofstream& ofs)
        {
            std::vector<uint8_t> buffer(header.exportTableSize);
            uint8_t* dataPtr = buffer.data();

            *(uint32_t*)(dataPtr) = static_cast<uint32_t>(serializer->mExportTable.size());
            dataPtr += sizeof(uint32_t);
            for (auto it : serializer->mExportTable)
            {
                *(Guid*)(dataPtr) = it;
                dataPtr += sizeof(Guid);
            }
            ofs.write((const char*)(buffer.data()), header.exportTableSize);
        }

        static void writeObjectBuffers(AssetHeader& header, AssetSerializer* serializer, std::ofstream& ofs)
        {
            for (uint32_t i = 0; i < header.objectBufferCount; ++i)
            {
                AssetSerializer::ObjectBuffer& objectBuffer = serializer->mObjectBufferTable.at(i);
                uint32_t bufferSize = objectBuffer.buffer.size();
                ofs.write((char*)&bufferSize, sizeof(uint32_t));
                ofs.write((char*)objectBuffer.buffer.data(), bufferSize);
            }
        }
    };
}
