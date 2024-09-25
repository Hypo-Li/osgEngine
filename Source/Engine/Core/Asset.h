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
        uint64_t magic = 0x5445535341585858; // XXXASSET
        uint32_t version = 0;
        uint32_t flags = 0;
        Guid     guid;
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
                AssetHeader header;
                readAssetHeader(ifs, header);
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

        virtual ~Asset() = default;

        template <typename T = Object, std::enable_if_t<std::is_base_of_v<Object, T>, int> = 0>
        T* getRootObject()
        {
            return dynamic_cast<T*>(mRootObject.get());
        }

        inline bool isLoaded() const
        {
            return mIsLoaded;
        }

        inline bool needSave() const
        {
            return mNeedSave;
        }

        void setRootObject(Object* rootObject)
        {
            mRootObject = rootObject;
            if (rootObject)
            {
                mGuid = rootObject->getGuid();
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
            return mGuid;
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


            mNeedSave = false;
            mIsLoaded = true;
        }

        void forceSave()
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

            mNeedSave = false;
            mIsLoaded = true;
        }

        void addImportedObject(Object* object)
        {
            if (!object)
                return;
            mImportedObjects.insert(object->getGuid());
        }

        void addExportedObject(Object* object)
        {
            if (!object)
                return;
            mExportedObjects.insert(object);
        }

    protected:
        std::string mPath;
        Guid mGuid;
        bool mIsLoaded = false;
        bool mNeedSave = false;

        osg::ref_ptr<Object> mRootObject;

        // Asset 中需要保留导入表, 以供删除其它资产时查询本资产是否引用其对象
        std::unordered_set<Guid> mImportedObjects;
        // Asset 中需要保留导出表;
        std::unordered_set<osg::ref_ptr<Object>> mExportedObjects;

        static void readAssetHeader(std::ifstream& ifs, AssetHeader& header)
        {
            ifs.read((char*)&header, sizeof(AssetHeader));
        }

        static void readStringTable(std::ifstream& ifs, AssetSerializer* serializer, AssetHeader& header)
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
                serializer->addString(std::string((char*)dataPtr, stringLength));
                dataPtr += stringLength;
            }
        }

        static void readImportTable(std::ifstream& ifs, AssetSerializer* serializer, AssetHeader& header)
        {
            std::vector<uint8_t> buffer(header.importTableSize);
            uint8_t* dataPtr = buffer.data();
            ifs.read((char*)(dataPtr), header.importTableSize);

            uint32_t importCount = *(uint32_t*)(dataPtr);
            dataPtr += sizeof(uint32_t);

            for (uint32_t i = 0; i < importCount; ++i)
            {
                ImportItem importItem = *(ImportItem*)(dataPtr);
                dataPtr += sizeof(ImportItem);
                serializer->addImportItem(importItem);
            }
        }

        static void readExportEable(std::ifstream& ifs, AssetSerializer* serializer, AssetHeader& header)
        {
            std::vector<uint8_t> buffer(header.exportTableSize);
            uint8_t* dataPtr = buffer.data();
            ifs.read((char*)(dataPtr), header.exportTableSize);

            uint32_t exportCount = *(uint32_t*)(dataPtr);
            dataPtr += sizeof(uint32_t);

            for (uint32_t i = 0; i < exportCount; ++i)
            {
                ExportItem exportItem = *(ExportItem*)(dataPtr);
                dataPtr += sizeof(ExportItem);
                serializer->addExportItem(exportItem);
            }
        }

        static void readObjectBuffers(std::ifstream& ifs, AssetSerializer* serializer, AssetHeader& header)
        {
            for (uint32_t i = 0; i < header.objectBufferCount; ++i)
            {
                uint32_t objectBufferSize = 0;
                ifs.read((char*)(&objectBufferSize), sizeof(uint32_t));
                uint32_t objectBufferIndex = serializer->createNewObjectBuffer(objectBufferSize);
                const ObjectBuffer& objectBuffer = serializer->getObjectBufferTable().at(objectBufferIndex);
                ifs.read((char*)(objectBuffer.getData()), objectBufferSize);
            }
        }

        static void writeAssetHeader(std::ofstream& ofs, AssetSerializer* serializer, AssetHeader& header)
        {
            serializer->fillAssetHeader(header);
            ofs.write((char*)(&header), sizeof(AssetHeader));
        }

        static void writeStringTable(std::ofstream& ofs, AssetSerializer* serializer, AssetHeader& header)
        {
            std::vector<uint8_t> buffer(header.stringTableSize);
            uint8_t* dataPtr = buffer.data();

            *(uint32_t*)(dataPtr) = static_cast<uint32_t>(serializer->getStringTable().size());
            dataPtr += sizeof(uint32_t);

            for (auto& it : serializer->getStringTable())
            {
                *(uint32_t*)(dataPtr) = static_cast<uint32_t>(it.size());
                dataPtr += sizeof(uint32_t);
                std::memcpy(dataPtr, it.data(), it.size());
                dataPtr += it.size();
            }

            ofs.write((const char*)(buffer.data()), header.stringTableSize);
        }

        static void writeImportTable(std::ofstream& ofs, AssetSerializer* serializer, AssetHeader& header)
        {
            std::vector<uint8_t> buffer(header.importTableSize);
            uint8_t* dataPtr = buffer.data();

            *(uint32_t*)(dataPtr) = static_cast<uint32_t>(serializer->getImportTable().size());
            dataPtr += sizeof(uint32_t);

            for (auto it : serializer->getImportTable())
            {
                *(ImportItem*)(dataPtr) = it;
                dataPtr += sizeof(ImportItem);
            }

            ofs.write((const char*)(buffer.data()), header.importTableSize);
        }

        static void writeExportTable(std::ofstream& ofs, AssetSerializer* serializer, AssetHeader& header)
        {
            std::vector<uint8_t> buffer(header.exportTableSize, 0);
            uint8_t* dataPtr = buffer.data();

            *(uint32_t*)(dataPtr) = static_cast<uint32_t>(serializer->getExportTable().size());
            dataPtr += sizeof(uint32_t);

            for (auto& it : serializer->getExportTable())
            {
                *(ExportItem*)(dataPtr) = it;
                dataPtr += sizeof(ExportItem);
            }

            ofs.write((const char*)(buffer.data()), header.exportTableSize);
        }

        static void writeObjectBuffers(std::ofstream& ofs, AssetSerializer* serializer, AssetHeader& header)
        {
            for (uint32_t i = 0; i < header.objectBufferCount; ++i)
            {
                const ObjectBuffer& objectBuffer = serializer->getObjectBufferTable().at(i);
                uint32_t bufferSize = objectBuffer.getSize();
                ofs.write((const char*)&bufferSize, sizeof(uint32_t));
                ofs.write((const char*)objectBuffer.getData(), bufferSize);
            }
        }
    };
}
