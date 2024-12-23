#include "AssetSerializer.h"
#include "../AssetManager.h"

namespace xxx
{
    using namespace refl;

    void AssetSerializer::fillAssetHeader(AssetHeader& header)
    {
        header.stringTableSize = sizeof(uint32_t);
        {
            for (auto& it : mStringTable)
                header.stringTableSize += sizeof(uint32_t) + it.size();
        }
        header.stringTableSize = (header.stringTableSize + 3) & ~3;
        header.importTableSize = sizeof(uint32_t) + sizeof(ImportItem) * mImportTable.size();
        header.exportTableSize = sizeof(uint32_t) + sizeof(ExportItem) * mExportTable.size();
        header.objectBufferCount = mObjectBufferTable.size();
    }

    uint32_t AssetSerializer::addString(const std::string& str)
    {
        auto findResult = std::find(mStringTable.begin(), mStringTable.end(), str);
        if (findResult == mStringTable.end())
        {
            mStringTable.emplace_back(str);
            return mStringTable.size() - 1;
        }
        return findResult - mStringTable.begin();
    }

    uint32_t AssetSerializer::addImportItem(const ImportItem& importItem)
    {
        auto findResult = std::find_if(mImportTable.begin(), mImportTable.end(),
            [importItem](ImportItem& item) {
                return item.objectGuid == importItem.objectGuid;
            }
        );
        if (findResult == mImportTable.end())
        {
            mImportTable.emplace_back(importItem);
            return mImportTable.size() - 1;
        }
        return findResult - mImportTable.begin();
    }

    uint32_t AssetSerializer::addExportItem(const ExportItem& exportItem)
    {
        auto findResult = std::find_if(mExportTable.begin(), mExportTable.end(),
            [exportItem](ExportItem& item) {
                return item.objectGuid == exportItem.objectGuid;
            }
        );
        if (findResult == mExportTable.end())
        {
            mExportTable.emplace_back(exportItem);
            return mExportTable.size() - 1;
        }
        return findResult - mExportTable.begin();
    }

    uint32_t AssetSerializer::createNewObjectBuffer(uint32_t bufferSize)
    {
        mObjectBufferTable.emplace_back(bufferSize);
        return mObjectBufferTable.size() - 1;
    }

    int32_t AssetSerializer::getIndexOfObject(Object* object)
    {
        int32_t index = 0;
        if (object == nullptr)
            index = 0;
        else if (object->getAsset() != mAsset && object->getAsset() != nullptr)
        {
            // imported object
            auto findResult = std::find_if(
                mImportTable.begin(), mImportTable.end(),
                [object](ImportItem& item) {
                    return item.objectGuid == object->getGuid();
                }
            );

            if (findResult == mImportTable.end())
            {
                // if no saved
                mImportTable.push_back({ object->getGuid(), addString(object->getAsset()->getPath()) });
                index = mImportTable.size() - 1;
            }
            else
            {
                // if saved
                index = findResult - mImportTable.begin();
            }

            index = index + 1;
        }
        else
        {
            // exported object
            auto findResult = std::find_if(
                mExportTable.begin(), mExportTable.end(),
                [object](ExportItem& item) {
                    return item.objectGuid == object->getGuid();
                }
            );

            if (findResult == mExportTable.end())
            {
                // if no saved
                index = mExportTable.size();
                std::string className(object->getClass()->getName());
                mExportTable.push_back({ object->getGuid(), addString(className) });
                mTempObjects[index] = object;

                pushObjectBufferIndex(createNewObjectBuffer());
                serializeObject(object);
                popObjectBufferIndex();
            }
            else
            {
                // if saved
                index = findResult - mExportTable.begin();
            }

            index = -(index + 1);
        }
        return index;
    }

    Object* AssetSerializer::getObjectByIndex(int32_t index)
    {
        Object* object;
        if (index == 0)
            object = nullptr;
        else if (index > 0)
        {
            // imported object
            index = index - 1;
            Guid guid = mImportTable[index].objectGuid;

            Asset* asset = AssetManager::get().getAsset(guid);
            if (!asset)
            {
                // try to use path get asset
                asset = AssetManager::get().getAsset(mStringTable[mImportTable[index].pathStrIndex]);
                // LogWarnning
            }
            if (!asset)
            {
                // LogError could not find asset getString(mImportTable[index].path)
                object = nullptr;
            }
            else
            {
                object = asset->getRootObject();
                //mAsset->addImportedObject(object);
            }
        }
        else
        {
            // exported object
            index = -index - 1;
            if (!mTempObjects.count(index))
            {
                // if no loaded
                Guid guid = mExportTable[index].objectGuid;
                Class* clazz = Reflection::getClass(getStringTable().at(mExportTable[index].classNameStrIndex));
                object = static_cast<Object*>(clazz->newInstance());
                object->setGuid(guid);
                mTempObjects[index] = object;
                //mAsset->addExportedObject(object);

                // object->mGuid = guid;
                pushObjectBufferIndex(index);
                serializeObject(object);
                popObjectBufferIndex();
            }
            else
            {
                // if loaded
                object = mTempObjects[index];
            }
        }
        return object;
    }

    void AssetSerializer::serializeFundamental(refl::Fundamental* fundamental, void* data, size_t count)
    {
        if (fundamental == Reflection::BoolType)
            serializeArithmetic((bool*)(data), count);
        else if (fundamental == Reflection::CharType)
            serializeArithmetic((char*)(data), count);
        else if (fundamental == Reflection::WCharType)
            serializeArithmetic((wchar_t*)(data), count);
        else if (fundamental == Reflection::Int8Type)
            serializeArithmetic((int8_t*)(data), count);
        else if (fundamental == Reflection::Int16Type)
            serializeArithmetic((int16_t*)(data), count);
        else if (fundamental == Reflection::Int32Type)
            serializeArithmetic((int32_t*)(data), count);
        else if (fundamental == Reflection::Int64Type)
            serializeArithmetic((int64_t*)(data), count);
        else if (fundamental == Reflection::Uint8Type)
            serializeArithmetic((uint8_t*)(data), count);
        else if (fundamental == Reflection::Uint16Type)
            serializeArithmetic((uint16_t*)(data), count);
        else if (fundamental == Reflection::Uint32Type)
            serializeArithmetic((uint32_t*)(data), count);
        else if (fundamental == Reflection::Uint64Type)
            serializeArithmetic((uint64_t*)(data), count);
        else if (fundamental == Reflection::FloatType)
            serializeArithmetic((float*)(data), count);
        else if (fundamental == Reflection::DoubleType)
            serializeArithmetic((double*)(data), count);
    }

    void AssetSerializer::serializeStructure(Structure* structure, void* data, size_t count)
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            void* structData = static_cast<uint8_t*>(data) + structure->getSize() * i;
            for (Property* prop : structure->getProperties())
                serializeType(prop->getDeclaredType(), prop->getValuePtr(structData));
        }
            
    }
}
