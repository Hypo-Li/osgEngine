#include "AssetLoader.h"
#include "../Asset.h"
#include "../AssetManager.h"

namespace xxx
{
    using namespace refl;
    void AssetLoader::serializeObject(Object*& object)
    {
        Class* clazz = object->getClass();

        uint32_t propertyCount;
        serialize(&propertyCount);

        std::vector<Property*> properties;
        Class* baseClass = clazz;
        while (baseClass)
        {
            const std::vector<Property*>& props = clazz->getProperties();
            properties.insert(properties.end(), props.begin(), props.end());
            baseClass = baseClass->getBaseClass();
        }

        for (size_t i = 0; i < propertyCount; ++i)
        {
            std::string propertyName;
            uint32_t propertySize;
            serializeStdString(&propertyName);
            serialize(&propertySize);

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

        object->postSerialize();
    }

    void AssetLoader::serializeBinary(void* data, uint32_t count)
    {
        ObjectBuffer& objectBuffer = getCurrentObjectBuffer();
        std::memcpy(data, objectBuffer.buffer.data() + objectBuffer.pointer, count);
        objectBuffer.pointer = objectBuffer.pointer + count;
    }

    template <typename T>
    static void setEnumValues(Enum* enumerate, void* data, const std::vector<std::string>& enumNames, uint32_t count)
    {
        for (uint32_t i = 0; i < count; ++i)
            ((T*)(data))[i] = static_cast<T>(enumerate->getValueByName(enumNames[i]));
    }

    void AssetLoader::serializeEnum(Enum* enumerate, void* data, uint32_t count)
    {
        std::vector<std::string> enumNames(count);
        Type* underlying = enumerate->getUnderlyingType();
        serializeStdString(enumNames.data(), count);
        if (underlying == Reflection::Int8Type)
            setEnumValues<int8_t>(enumerate, data, enumNames, count);
        else if (underlying == Reflection::Int16Type)
            setEnumValues<int16_t>(enumerate, data, enumNames, count);
        else if (underlying == Reflection::Int32Type)
            setEnumValues<int32_t>(enumerate, data, enumNames, count);
        else if (underlying == Reflection::Int64Type)
            setEnumValues<int64_t>(enumerate, data, enumNames, count);
        else if (underlying == Reflection::Uint8Type)
            setEnumValues<uint8_t>(enumerate, data, enumNames, count);
        else if (underlying == Reflection::Uint16Type)
            setEnumValues<uint16_t>(enumerate, data, enumNames, count);
        else if (underlying == Reflection::Uint32Type)
            setEnumValues<uint32_t>(enumerate, data, enumNames, count);
        else if (underlying == Reflection::Uint64Type)
            setEnumValues<uint64_t>(enumerate, data, enumNames, count);
    }

    void AssetLoader::serializeClass(Class* clazz, void* data, uint32_t count)
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            Object*& object = (static_cast<Object**>(data))[i];
            int32_t index;
            if (currentObjectBufferIsValid())
                serialize(&index);
            else
                index = -1;
            if (index > 0)
            {
                // imported object
                index = index - 1;
                uint32_t pathStringIndex = mImportTable[index];
                const std::string& path = mStringTable.at(pathStringIndex);

                Asset* asset = AssetManager::get().getAsset(path);
                object = asset->getRootObject();
            }
            else
            {
                // exported object
                index = -index - 1;
                if (index <= mObjectsTemp.size())
                {
                    // if no loaded
                    Guid guid = mExportTable[index];
                    object = static_cast<Object*>(clazz->newInstance());
                    // object->mGuid = guid;
                    pushObjectBufferIndex(index);
                    serializeObject(object);
                    popObjectBufferIndex();

                    mObjectsTemp.push_back(object);
                }
                else
                {
                    // if loaded
                    object = mObjectsTemp[index];
                }
            }
        }
    }

    void AssetLoader::serializeStdString(std::string* data, uint32_t count)
    {
        std::vector<uint32_t> stringIndices(count);
        serialize(stringIndices.data(), count);
        for (uint32_t i = 0; i < count; ++i)
        {
            data[i] = mStringTable.at(stringIndices[i]);
        }
    }

    void AssetLoader::serializeStdMap(StdMap* stdMap, void* data, uint32_t count)
    {
        Type* keyType = stdMap->getKeyType();
        Type* valueType = stdMap->getValueType();
        for (uint32_t i = 0; i < count; ++i)
        {
            void* stdMapData = static_cast<uint8_t*>(data) + stdMap->getSize() * i;
            size_t keyValuePairCount;
            serialize(&keyValuePairCount);

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

    void AssetLoader::serializeStdSet(StdSet* stdSet, void* data, uint32_t count)
    {
        Type* elementType = stdSet->getElementType();
        const size_t stdSetSize = stdSet->getSize();
        for (uint32_t i = 0; i < count; ++i)
        {
            void* stdSetData = static_cast<uint8_t*>(data) + stdSetSize * i;
            size_t elementCount;
            serialize(&elementCount);

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

    void AssetLoader::serializeStdVariant(StdVariant* stdVariant, void* data, uint32_t count)
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            void* stdVariantData = static_cast<uint8_t*>(data) + stdVariant->getSize() * i;
            uint32_t typeIndex;
            serialize(&typeIndex);
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

    void AssetLoader::serializeStdVector(StdVector* stdVector, void* data, uint32_t count)
    {
        Type* elementType = stdVector->getElementType();
        for (uint32_t i = 0; i < count; ++i)
        {
            void* stdVectorData = static_cast<uint8_t*>(data) + stdVector->getSize() * i;
            size_t elementCount;
            serialize(&elementCount);
            if (elementCount > 0)
            {
                stdVector->resize(stdVectorData, elementCount);
                serializeType(elementType, stdVector->getElementPtrByIndex(stdVectorData, 0), elementCount);
            }
        }
    }
}
