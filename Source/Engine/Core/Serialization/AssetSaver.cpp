#include "AssetSaver.h"
#include "../Asset.h"

namespace xxx
{
    using namespace refl;

    void AssetSaver::serializeObject(Object* object)
    {
        object->preSerialize(this);

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

        object->postSerialize(this);
    }

    void AssetSaver::serializeBinary(void* data, size_t count)
    {
        ObjectBuffer& objectBuffer = getCurrentObjectBuffer();
        objectBuffer.writeData(data, count);
    }

    void AssetSaver::serializeEnum(Enum* enumerate, void* data, size_t count)
    {
        std::vector<std::string> valueNames(count);
        for (size_t i = 0; i < count; ++i)
            valueNames[i] = enumerate->getNameByValue(enumerate->getValueByValuePtr(data));
        serializeStdString(valueNames.data(), count);
    }

    void AssetSaver::serializeClass(Object** data, size_t count)
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

    void AssetSaver::serializeStdString(std::string* data, size_t count)
    {
        std::vector<uint32_t> stringIndices(count);
        for (uint32_t i = 0; i < count; ++i)
        {
            stringIndices[i] = addString(data[i]);
        }
        serializeArithmetic(stringIndices.data(), count);
    }

    void AssetSaver::serializeStdMap(StdMap* stdMap, void* data, size_t count)
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

    void AssetSaver::serializeStdSet(StdSet* stdSet, void* data, size_t count)
    {
        Type* elementType = stdSet->getElementType();
        for (uint32_t i = 0; i < count; ++i)
        {
            void* stdSetData = static_cast<uint8_t*>(data) + stdSet->getSize() * i;
            std::vector<const void*> elementPtrs = stdSet->getElementPtrs(stdSetData);
            size_t elementCount = elementPtrs.size();
            serializeArithmetic(&elementCount);
            for (size_t j = 0; j < elementCount; ++j)
                serializeType(elementType, const_cast<void*>(elementPtrs[j]));
        }
    }

    void AssetSaver::serializeStdUnorderedMap(refl::StdUnorderedMap* stdUnorderedMap, void* data, size_t count)
    {
        Type* keyType = stdUnorderedMap->getKeyType();
        Type* valueType = stdUnorderedMap->getValueType();
        for (uint32_t i = 0; i < count; ++i)
        {
            void* stdUnorderedMapData = static_cast<uint8_t*>(data) + stdUnorderedMap->getSize() * i;
            std::vector<std::pair<const void*, void*>> keyValuePtrs = stdUnorderedMap->getKeyValuePtrs(stdUnorderedMapData);
            size_t keyValuePairCount = keyValuePtrs.size();
            serializeArithmetic(&keyValuePairCount);
            for (size_t j = 0; j < keyValuePairCount; ++j)
                serializeType(keyType, const_cast<void*>(keyValuePtrs[j].first));
            for (size_t j = 0; j < keyValuePairCount; ++j)
                serializeType(valueType, keyValuePtrs[j].second);
        }
    }

    void AssetSaver::serializeStdUnorderedSet(refl::StdUnorderedSet* stdUnorderedSet, void* data, size_t count)
    {
        Type* elementType = stdUnorderedSet->getElementType();
        for (uint32_t i = 0; i < count; ++i)
        {
            void* stdSetData = static_cast<uint8_t*>(data) + stdUnorderedSet->getSize() * i;
            std::vector<const void*> elementPtrs = stdUnorderedSet->getElementPtrs(stdSetData);
            size_t elementCount = elementPtrs.size();
            serializeArithmetic(&elementCount);
            for (size_t j = 0; j < elementCount; ++j)
                serializeType(elementType, const_cast<void*>(elementPtrs[j]));
        }
    }

    void AssetSaver::serializeStdVariant(StdVariant* stdVariant, void* data, size_t count)
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

    void AssetSaver::serializeStdVector(StdVector* stdVector, void* data, size_t count)
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
