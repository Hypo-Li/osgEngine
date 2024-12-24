#include "AssetSaver.h"
#include "../Asset.h"

namespace xxx
{
    using namespace refl;

    AssetSaver::AssetSaver(Asset* asset) : AssetSerializer(asset) {}

    void AssetSaver::serializeEnumeration(Enumeration* enumeration, void* data, size_t count)
    {
        std::vector<std::string> valueNames(count);
        for (size_t i = 0; i < count; ++i)
            valueNames[i] = enumeration->getNameByValue(enumeration->getValue(data));
        serializeStdString(nullptr, valueNames.data(), count);
    }

    void AssetSaver::serializeClass(Class* clazz, void* data, size_t count)
    {
        Object** objects = static_cast<Object**>(data);
        for (size_t i = 0; i < count; ++i)
        {
            Object* object = objects[i];
            int32_t objectIndex;
            if (object == nullptr)
                objectIndex = 0;
            else
            {
                Asset* asset = object->getAsset();
                if (asset != getAsset() && asset != nullptr)
                {
                    if (object->getOwner() != nullptr)
                    {
                        //LOG_ERROR("Cannot reference object that is not asset root object");
                        objectIndex = 0;
                    }
                    else
                    {
                        uint32_t importIndex = addImportItem(addString(asset->getPath()));
                        objectIndex = importIndex + 1;
                    }
                }
                else
                {
                    uint32_t exportIndex;
                    auto exportIndexFindResult = mExportIndexTemp.find(object);
                    if (exportIndexFindResult == mExportIndexTemp.end())
                    {
                        exportIndex = createObjectBuffer();
                        mExportIndexTemp.emplace(object, exportIndex);

                        pushObjectBuffer(exportIndex);
                        std::string className(object->getClass()->getName());
                        serializeStdString(nullptr, &className);
                        serializeObject(object);
                        popObjectBuffer();
                    }
                    else
                    {
                        exportIndex = exportIndexFindResult->second;
                    }
                    objectIndex = -int32_t(exportIndex + 1);
                }
            }
            if (getCurrentObjectBuffer())
                serializeArithmetic(&objectIndex);
        }
    }

    void AssetSaver::serializeStdArray(StdArray* stdArray, void* data, size_t count)
    {
        Type* elementType = stdArray->getElementType();
        for (size_t i = 0; i < count; ++i)
        {
            void* stdArrayData = static_cast<uint8_t*>(data) + stdArray->getSize() * i;
            serializeType(elementType, stdArray->getElementPtr(stdArrayData, 0), stdArray->getElementCount());
        }
    }

    void AssetSaver::serializeStdList(StdList* stdList, void* data, size_t count)
    {
        Type* elementType = stdList->getElementType();
        for (size_t i = 0; i < count; ++i)
        {
            void* stdListData = static_cast<uint8_t*>(data) + stdList->getSize() * i;
            std::vector<void*> elementPtrs = stdList->getElementPtrs(stdListData);
            size_t elementCount = elementPtrs.size();
            serializeArithmetic(&elementCount);
            for (size_t j = 0; j < elementCount; ++j)
                serializeType(elementType, static_cast<void*>(elementPtrs[j]));
        }
    }

    void AssetSaver::serializeStdMap(StdMap* stdMap, void* data, size_t count)
    {
        Type* keyType = stdMap->getKeyType();
        Type* valueType = stdMap->getValueType();
        for (size_t i = 0; i < count; ++i)
        {
            void* stdMapData = static_cast<uint8_t*>(data) + stdMap->getSize() * i;
            size_t keyValuePairCount = stdMap->getKeyValuePairCount(stdMapData);
            serializeArithmetic(&keyValuePairCount);
            std::vector<const void*> keys = stdMap->getKeyPtrs(stdMapData);
            for (size_t j = 0; j < keyValuePairCount; ++j)
                serializeType(keyType, const_cast<void*>(keys[j]));
            std::vector<void*> values = stdMap->getValuePtrs(stdMapData);
            for (size_t j = 0; j < keyValuePairCount; ++j)
                serializeType(valueType, values[j]);
        }
    }

    void AssetSaver::serializeStdPair(StdPair* stdPair, void* data, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
        {
            void* stdPairData = static_cast<uint8_t*>(data) + stdPair->getSize() * i;
            serializeType(stdPair->getFirstType(), stdPair->getFirstPtr(stdPairData));
            serializeType(stdPair->getSecondType(), stdPair->getSecondPtr(stdPairData));
        }
    }

    void AssetSaver::serializeStdSet(StdSet* stdSet, void* data, size_t count)
    {
        Type* elementType = stdSet->getElementType();
        for (size_t i = 0; i < count; ++i)
        {
            void* stdSetData = static_cast<uint8_t*>(data) + stdSet->getSize() * i;
            std::vector<const void*> elementPtrs = stdSet->getElementPtrs(stdSetData);
            size_t elementCount = elementPtrs.size();
            serializeArithmetic(&elementCount);
            for (size_t j = 0; j < elementCount; ++j)
                serializeType(elementType, const_cast<void*>(elementPtrs[j]));
        }
    }

    void AssetSaver::serializeStdString(StdString* stdString, void* data, size_t count)
    {
        std::vector<uint32_t> stringIndices(count);
        for (size_t i = 0; i < count; ++i)
            stringIndices[i] = addString(static_cast<std::string*>(data)[i]);
        serializeArithmetic(stringIndices.data(), count);
    }

    void AssetSaver::serializeStdUnorderedMap(refl::StdUnorderedMap* stdUnorderedMap, void* data, size_t count)
    {
        Type* keyType = stdUnorderedMap->getKeyType();
        Type* valueType = stdUnorderedMap->getValueType();
        for (size_t i = 0; i < count; ++i)
        {
            void* stdUnorderedMapData = static_cast<uint8_t*>(data) + stdUnorderedMap->getSize() * i;
            size_t keyValuePairCount = stdUnorderedMap->getKeyValuePairCount(stdUnorderedMapData);
            serializeArithmetic(&keyValuePairCount);
            std::vector<const void*> keys = stdUnorderedMap->getKeyPtrs(stdUnorderedMapData);
            for (size_t j = 0; j < keyValuePairCount; ++j)
                serializeType(keyType, const_cast<void*>(keys[j]));
            std::vector<void*> values = stdUnorderedMap->getValuePtrs(stdUnorderedMapData);
            for (size_t j = 0; j < keyValuePairCount; ++j)
                serializeType(valueType, values[j]);
        }
    }

    void AssetSaver::serializeStdUnorderedSet(refl::StdUnorderedSet* stdUnorderedSet, void* data, size_t count)
    {
        Type* elementType = stdUnorderedSet->getElementType();
        for (size_t i = 0; i < count; ++i)
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
        for (size_t i = 0; i < count; ++i)
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
        for (size_t i = 0; i < count; ++i)
        {
            void* stdVectorData = static_cast<uint8_t*>(data) + stdVector->getSize() * i;
            size_t elementCount = stdVector->getElementCount(stdVectorData);
            serializeArithmetic(&elementCount);
            if (elementCount > 0)
                serializeType(elementType, stdVector->getElementPtr(stdVectorData, 0), elementCount);
        }
    }

    void AssetSaver::serializeBinary(void* data, size_t size)
    {
        getCurrentObjectBuffer()->write(data, size);
    }

    void AssetSaver::serializeObject(Object* object)
    {
        object->preSave();

        ObjectBuffer* objectBuffer = getCurrentObjectBuffer();

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

            serializeStdString(nullptr, &propertyName);
            uint32_t propertySizePos = objectBuffer->tell();
            serializeArithmetic(&propertySize);

            uint32_t propertyBeginPos = objectBuffer->tell();
            void* valuePtr = prop->getValuePtr(object);
            serializeType(prop->getDeclaredType(), valuePtr);
            uint32_t propertyEndPos = objectBuffer->tell();

            propertySize = propertyEndPos - propertyBeginPos;
            objectBuffer->seek(propertySizePos);
            serializeArithmetic(&propertySize);
            objectBuffer->seek(propertyEndPos);
        }

        object->postSave();
    }
}
