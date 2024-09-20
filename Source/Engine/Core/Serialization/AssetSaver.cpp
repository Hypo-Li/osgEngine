#include "AssetSaver.h"
#include "../Asset.h"

namespace xxx
{
    using namespace refl;

    void AssetSaver::serializeObject(Object* object)
    {
        object->preSerialize();

        Class* clazz = object->getClass();
        const Object* defaultObject = clazz->getDefaultObject();
        std::vector<Property*> properties;
        Class* baseClass = clazz;
        while (baseClass)
        {
            for (Property* prop : baseClass->getProperties())
            {
                if (!prop->compare(defaultObject, object))
                    properties.emplace_back(prop);
            }
            baseClass = baseClass->getBaseClass();
        }
        uint32_t propertyCount = properties.size();
        serialize(&propertyCount);

        for (Property* prop : properties)
        {
            std::string propertyName(prop->getName());
            uint32_t propertySize = 0;

            serializeStdString(&propertyName);
            uint32_t propertySizePos = tell();
            serialize(&propertySize);

            uint32_t propertyBeginPos = tell();
            void* valuePtr = prop->getValuePtr(object);
            serializeType(prop->getDeclaredType(), valuePtr);
            uint32_t propertyEndPos = tell();

            propertySize = propertyEndPos - propertyBeginPos;
            seek(propertySizePos);
            serialize(&propertySize);
            seek(propertyEndPos);
        }
    }

    void AssetSaver::serializeBinary(void* data, uint32_t count)
    {
        ObjectBuffer& objectBuffer = getCurrentObjectBuffer();
        size_t newPointer = objectBuffer.pointer + count;
        if (newPointer > objectBuffer.buffer.size())
            objectBuffer.buffer.resize(newPointer);
        std::memcpy(objectBuffer.buffer.data() + objectBuffer.pointer, data, count);
        objectBuffer.pointer = newPointer;
    }

    template <typename T>
    static void getEnumNames(Enum* enumerate, void* data, std::vector<std::string>& enumNames, uint32_t count)
    {
        for (uint32_t i = 0; i < count; ++i)
            enumNames[i] = enumerate->getNameByValue(static_cast<int64_t>(((T*)(data))[i]));
    }

    void AssetSaver::serializeEnum(Enum* enumerate, void* data, uint32_t count)
    {
        std::vector<std::string> enumNames(count);
        Type* underlying = enumerate->getUnderlyingType();
        if (underlying == Reflection::Int8Type)
            getEnumNames<int8_t>(enumerate, data, enumNames, count);
        else if (underlying == Reflection::Int16Type)
            getEnumNames<int16_t>(enumerate, data, enumNames, count);
        else if (underlying == Reflection::Int32Type)
            getEnumNames<int32_t>(enumerate, data, enumNames, count);
        else if (underlying == Reflection::Int64Type)
            getEnumNames<int64_t>(enumerate, data, enumNames, count);
        else if (underlying == Reflection::Uint8Type)
            getEnumNames<uint8_t>(enumerate, data, enumNames, count);
        else if (underlying == Reflection::Uint16Type)
            getEnumNames<uint16_t>(enumerate, data, enumNames, count);
        else if (underlying == Reflection::Uint32Type)
            getEnumNames<uint32_t>(enumerate, data, enumNames, count);
        else if (underlying == Reflection::Uint64Type)
            getEnumNames<uint64_t>(enumerate, data, enumNames, count);
        serializeStdString(enumNames.data(), count);
    }

    void AssetSaver::serializeClass(Class* clazz, void* data, uint32_t count)
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            Object*& object = (static_cast<Object**>(data))[i];
            std::string className(object->getClass()->getName());
            
            
            int32_t index;
            if (object->getAsset() != mAsset && object->getAsset() != nullptr)
            {
                // imported object
                auto findResult = std::find(mImportTable.begin(), mImportTable.end(), object->getGuid());

                if (findResult == mImportTable.end())
                {
                    // if no saved
                    mImportTable.emplace_back(object->getGuid());
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
                auto findResult = std::find(mExportTable.begin(), mExportTable.end(), object->getGuid());

                if (findResult == mExportTable.end())
                {
                    // if no saved
                    index = mExportTable.size();
                    mExportTable.emplace_back(object->getGuid());

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
            if (currentObjectBufferIsValid())
            {
                serialize(&index);
            }
        }
    }

    void AssetSaver::serializeStdString(std::string* data, uint32_t count)
    {
        std::vector<uint32_t> stringIndices(count);
        for (uint32_t i = 0; i < count; ++i)
        {
            auto findResult = std::find(mStringTable.begin(), mStringTable.end(), data[i]);
            if (findResult == mStringTable.end())
            {
                mStringTable.emplace_back(data[i]);
                stringIndices[i] = mStringTable.size() - 1;
            }
            else
            {
                stringIndices[i] = findResult - mStringTable.begin();
            }
        }
        serialize(stringIndices.data(), count);
    }

    void AssetSaver::serializeStdMap(StdMap* stdMap, void* data, uint32_t count)
    {
        Type* keyType = stdMap->getKeyType();
        Type* valueType = stdMap->getValueType();
        for (uint32_t i = 0; i < count; ++i)
        {
            void* stdMapData = static_cast<uint8_t*>(data) + stdMap->getSize() * i;
            std::vector<std::pair<const void*, void*>> keyValuePtrs = stdMap->getKeyValuePtrs(stdMapData);
            size_t keyValuePairCount = keyValuePtrs.size();
            serialize(&keyValuePairCount);
            for (size_t j = 0; j < keyValuePairCount; ++j)
                serializeType(keyType, const_cast<void*>(keyValuePtrs[j].first));
            for (size_t j = 0; j < keyValuePairCount; ++j)
                serializeType(valueType, keyValuePtrs[j].second);
        }
    }

    void AssetSaver::serializeStdSet(StdSet* stdSet, void* data, uint32_t count)
    {
        Type* elementType = stdSet->getElementType();
        const size_t stdSetSize = stdSet->getSize();
        for (uint32_t i = 0; i < count; ++i)
        {
            void* stdSetData = static_cast<uint8_t*>(data) + stdSetSize * i;
            std::vector<const void*> elementPtrs = stdSet->getElementPtrs(stdSetData);
            size_t elementCount = elementPtrs.size();
            serialize(&elementCount);
            for (size_t j = 0; j < elementCount; ++j)
                serializeType(elementType, const_cast<void*>(elementPtrs[j]));
        }
    }

    void AssetSaver::serializeStdVariant(StdVariant* stdVariant, void* data, uint32_t count)
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            void* stdVariantData = static_cast<uint8_t*>(data) + stdVariant->getSize() * i;
            uint32_t typeIndex = stdVariant->getTypeIndex(stdVariantData);
            serialize(&typeIndex);
            Type* variantType = stdVariant->getTypes().at(typeIndex);
            void* valuePtr = stdVariant->getValuePtr(stdVariantData);
            serializeType(variantType, valuePtr);
        }
    }

    void AssetSaver::serializeStdVector(StdVector* stdVector, void* data, uint32_t count)
    {
        Type* elementType = stdVector->getElementType();
        for (uint32_t i = 0; i < count; ++i)
        {
            void* stdVectorData = static_cast<uint8_t*>(data) + stdVector->getSize() * i;
            size_t elementCount = stdVector->getElementCount(stdVectorData);
            serialize(&elementCount);
            if (elementCount > 0)
                serializeType(elementType, stdVector->getElementPtrByIndex(stdVectorData, 0), elementCount);
        }
    }
}
