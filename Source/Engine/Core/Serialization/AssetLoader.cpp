#include "AssetLoader.h"
#include "../Asset.h"
#include "../AssetManager.h"

namespace xxx
{
    using namespace refl;
    void AssetLoader::serializeObject(Object* object)
    {
        object->preSerialize(this);

        Class* clazz = object->getClass();

        uint32_t propertyCount;
        serializeArithmetic(&propertyCount);

        std::vector<Property*> properties;
        Class* baseClass = clazz;
        while (baseClass)
        {
            const std::vector<Property*>& props = baseClass->getProperties();
            properties.insert(properties.end(), props.begin(), props.end());
            baseClass = baseClass->getBaseClass();
        }

        for (size_t i = 0; i < propertyCount; ++i)
        {
            std::string propertyName;
            uint32_t propertySize;
            serializeStdString(&propertyName);
            serializeArithmetic(&propertySize);

            auto findResult = std::find_if(properties.begin(), properties.end(),
                [propertyName](const Property* prop)->bool
                {
                return propertyName == prop->getName();
                }
            );
            if (findResult != properties.end())
            {
                // found, serialize this property
                Property* prop = *findResult;
                properties.erase(findResult);

                void* valuePtr = prop->getValuePtr(object);
                serializeType(prop->getDeclaredType(), valuePtr);
            }
            else
            {
                // not found, skip this property
                seek(tell() + propertySize);
            }
        }

        object->postSerialize(this);
    }

    void AssetLoader::serializeBinary(void* data, size_t count)
    {
        ObjectBuffer& objectBuffer = getCurrentObjectBuffer();
        objectBuffer.readData(data, count);
    }

    void AssetLoader::serializeEnum(Enum* enumerate, void* data, size_t count)
    {
        std::vector<std::string> valueNames(count);
        serializeStdString(valueNames.data(), count);
        for (size_t i = 0; i < count; ++i)
            enumerate->setValue(data, enumerate->getValueByName(valueNames[i]));
    }

    void AssetLoader::serializeClass(Object** data, size_t count)
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            Object*& object = data[i];
            int32_t index;
            if (currentObjectBufferIsValid())
                serializeArithmetic(&index);
            else
                index = -1;
            object = getObjectByIndex(index);
        }
    }

    void AssetLoader::serializeStdString(std::string* data, size_t count)
    {
        std::vector<uint32_t> stringIndices(count);
        serializeArithmetic(stringIndices.data(), count);
        for (uint32_t i = 0; i < count; ++i)
        {
            data[i] = getStringTable().at(stringIndices[i]);
        }
    }

    void AssetLoader::serializeStdMap(StdMap* stdMap, void* data, size_t count)
    {
        Type* keyType = stdMap->getKeyType();
        Type* valueType = stdMap->getValueType();
        for (uint32_t i = 0; i < count; ++i)
        {
            void* stdMapData = static_cast<uint8_t*>(data) + stdMap->getSize() * i;
            size_t keyValuePairCount;
            serializeArithmetic(&keyValuePairCount);

            const bool keyTypeIsClass = keyType->getKind() == Type::Kind::Class;
            const bool valueTypeIsClass = valueType->getKind() == Type::Kind::Class;
            void* keys = keyTypeIsClass ?
                new Object * [keyValuePairCount] :
                keyType->newInstances(keyValuePairCount);
            void* values = valueTypeIsClass ?
                new Object * [keyValuePairCount] :
                valueType->newInstances(keyValuePairCount);
            serializeType(keyType, keys, keyValuePairCount);
            serializeType(valueType, values, keyValuePairCount);

            const size_t keySize = keyTypeIsClass ? sizeof(Object*) : keyType->getSize();
            const size_t valueSize = valueTypeIsClass ? sizeof(Object*) : valueType->getSize();
            for (size_t j = 0; j < keyValuePairCount; ++j)
            {
                void* key = static_cast<uint8_t*>(keys) + j * keySize;
                void* value = static_cast<uint8_t*>(values) + j * valueSize;
                stdMap->insertKeyValuePair(stdMapData, key, value);
            }

            if (keyTypeIsClass)
                delete[] static_cast<Object**>(keys);
            else
                keyType->deleteInstances(keys);
            if (valueTypeIsClass)
                delete[] static_cast<Object**>(values);
            else
                valueType->deleteInstances(values);
        }
    }

    void AssetLoader::serializeStdSet(StdSet* stdSet, void* data, size_t count)
    {
        Type* elementType = stdSet->getElementType();
        for (uint32_t i = 0; i < count; ++i)
        {
            void* stdSetData = static_cast<uint8_t*>(data) + stdSet->getSize() * i;
            size_t elementCount;
            serializeArithmetic(&elementCount);

            const bool elementTypeIsClass = elementType->getKind() == Type::Kind::Class;
            void* elements = elementTypeIsClass ?
                new Object * [elementCount] :
                elementType->newInstances(elementCount);
            serializeType(elementType, elements, elementCount);

            const size_t elementSize = elementTypeIsClass ? sizeof(Object*) : elementType->getSize();
            for (size_t j = 0; j < elementCount; ++j)
            {
                void* element = static_cast<uint8_t*>(elements) + j * elementSize;
                stdSet->insertElement(stdSetData, element);
            }

            if (elementTypeIsClass)
                delete[] static_cast<Object**>(elements);
            else
                elementType->deleteInstances(elements);
        }
    }

    void AssetLoader::serializeStdUnorderedMap(refl::StdUnorderedMap* stdUnorderedMap, void* data, size_t count)
    {
        Type* keyType = stdUnorderedMap->getKeyType();
        Type* valueType = stdUnorderedMap->getValueType();
        for (uint32_t i = 0; i < count; ++i)
        {
            void* stdUnorderedMapData = static_cast<uint8_t*>(data) + stdUnorderedMap->getSize() * i;
            size_t keyValuePairCount;
            serializeArithmetic(&keyValuePairCount);

            const bool keyTypeIsClass = keyType->getKind() == Type::Kind::Class;
            const bool valueTypeIsClass = valueType->getKind() == Type::Kind::Class;
            void* keys = keyTypeIsClass ?
                new Object * [keyValuePairCount] :
                keyType->newInstances(keyValuePairCount);
            void* values = valueTypeIsClass ?
                new Object * [keyValuePairCount] :
                valueType->newInstances(keyValuePairCount);
            serializeType(keyType, keys, keyValuePairCount);
            serializeType(valueType, values, keyValuePairCount);

            const size_t keySize = keyTypeIsClass ? sizeof(Object*) : keyType->getSize();
            const size_t valueSize = valueTypeIsClass ? sizeof(Object*) : valueType->getSize();
            for (size_t j = 0; j < keyValuePairCount; ++j)
            {
                void* key = static_cast<uint8_t*>(keys) + j * keySize;
                void* value = static_cast<uint8_t*>(values) + j * valueSize;
                stdUnorderedMap->insertKeyValuePair(stdUnorderedMapData, key, value);
            }

            if (keyTypeIsClass)
                delete[] static_cast<Object**>(keys);
            else
                keyType->deleteInstances(keys);
            if (valueTypeIsClass)
                delete[] static_cast<Object**>(values);
            else
                valueType->deleteInstances(values);
        }
    }

    void AssetLoader::serializeStdUnorderedSet(refl::StdUnorderedSet* stdUnorderedSet, void* data, size_t count)
    {
        Type* elementType = stdUnorderedSet->getElementType();
        for (uint32_t i = 0; i < count; ++i)
        {
            void* stdUnorderedSetData = static_cast<uint8_t*>(data) + stdUnorderedSet->getSize() * i;
            size_t elementCount;
            serializeArithmetic(&elementCount);

            const bool elementTypeIsClass = elementType->getKind() == Type::Kind::Class;
            void* elements = elementTypeIsClass ?
                new Object * [elementCount] :
                elementType->newInstances(elementCount);
            serializeType(elementType, elements, elementCount);

            const size_t elementSize = elementTypeIsClass ? sizeof(Object*) : elementType->getSize();
            for (size_t j = 0; j < elementCount; ++j)
            {
                void* element = static_cast<uint8_t*>(elements) + j * elementSize;
                stdUnorderedSet->insertElement(stdUnorderedSetData, element);
            }

            if (elementTypeIsClass)
                delete[] static_cast<Object**>(elements);
            else
                elementType->deleteInstances(elements);
        }
    }

    void AssetLoader::serializeStdVariant(StdVariant* stdVariant, void* data, size_t count)
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            void* stdVariantData = static_cast<uint8_t*>(data) + stdVariant->getSize() * i;
            uint32_t typeIndex;
            serializeArithmetic(&typeIndex);
            Type* variantType = stdVariant->getTypes().at(typeIndex);
            if (variantType->getKind() == Type::Kind::Class)
            {
                Object* object;
                serializeType(variantType, &object);
                stdVariant->setValueByTypeIndex(stdVariantData, typeIndex, &object);
            }
            else
            {
                void* valuePtr = variantType->newInstance();
                serializeType(variantType, valuePtr);
                stdVariant->setValueByTypeIndex(stdVariantData, typeIndex, valuePtr);
                variantType->deleteInstance(valuePtr);
            }
        }
    }

    void AssetLoader::serializeStdVector(StdVector* stdVector, void* data, size_t count)
    {
        Type* elementType = stdVector->getElementType();
        for (uint32_t i = 0; i < count; ++i)
        {
            void* stdVectorData = static_cast<uint8_t*>(data) + stdVector->getSize() * i;
            size_t elementCount;
            serializeArithmetic(&elementCount);
            if (elementCount > 0)
            {
                stdVector->resize(stdVectorData, elementCount);
                serializeType(elementType, stdVector->getElementPtrByIndex(stdVectorData, 0), elementCount);
            }
        }
    }
}
