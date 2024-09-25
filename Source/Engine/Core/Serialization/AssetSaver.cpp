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
        serializeArithmetic(&propertyCount);

        for (Property* prop : properties)
        {
            std::string propertyName(prop->getName());
            uint32_t propertySize = 0;

            serializeStdString(&propertyName);
            uint32_t propertySizePos = tell();
            serializeArithmetic(&propertySize);

            uint32_t propertyBeginPos = tell();
            void* valuePtr = prop->getValuePtr(object);
            serializeType(prop->getDeclaredType(), valuePtr);
            uint32_t propertyEndPos = tell();

            propertySize = propertyEndPos - propertyBeginPos;
            seek(propertySizePos);
            serializeArithmetic(&propertySize);
            seek(propertyEndPos);
        }

        object->postSerialize();
    }

    void AssetSaver::serializeBinary(void* data, uint32_t count)
    {
        ObjectBuffer& objectBuffer = getCurrentObjectBuffer();
        objectBuffer.writeData(data, count);
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

    void AssetSaver::serializeClass(Object** data, uint32_t count)
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            Object* object = data[i];

            int32_t index = getIndexOfObject(object);

            if (currentObjectBufferIsValid())
            {
                serializeArithmetic(&index);
            }
        }
    }

    void AssetSaver::serializeStdString(std::string* data, uint32_t count)
    {
        std::vector<uint32_t> stringIndices(count);
        for (uint32_t i = 0; i < count; ++i)
        {
            stringIndices[i] = addString(data[i]);
        }
        serializeArithmetic(stringIndices.data(), count);
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
            serializeArithmetic(&keyValuePairCount);
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
            serializeArithmetic(&elementCount);
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
            serializeArithmetic(&typeIndex);
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
            serializeArithmetic(&elementCount);
            if (elementCount > 0)
                serializeType(elementType, stdVector->getElementPtrByIndex(stdVectorData, 0), elementCount);
        }
    }
}
