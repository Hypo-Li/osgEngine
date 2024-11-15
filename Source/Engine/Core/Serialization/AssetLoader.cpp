#include "AssetLoader.h"
#include "../Asset.h"
#include "../AssetManager.h"

namespace xxx
{
    using namespace refl;
    void AssetLoader::serializeObject(Object* object)
    {
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
                // found and serialize this property
                Property* prop = *findResult;
                properties.erase(findResult);

                Type* declaredType = prop->getDeclaredType();
                if (declaredType->getKind() == Type::Kind::Class)
                {
                    Object* propObject;
                    serializeType(declaredType, &propObject);
                    prop->setValue(object, &propObject);
                }
                else
                {
                    serializeType(declaredType, prop->getValuePtr(object));
                }
            }
            else
            {
                // not found, skip this property
                seek(tell() + propertySize);
            }
        }

        object->postLoad();
    }

    void AssetLoader::serializeBinary(void* data, size_t count)
    {
        ObjectBuffer& objectBuffer = getCurrentObjectBuffer();
        objectBuffer.readData(data, count);
    }

    void AssetLoader::serializeEnumeration(Enumeration* enumeration, void* data, size_t count)
    {
        std::vector<std::string> valueNames(count);
        serializeStdString(valueNames.data(), count);
        for (size_t i = 0; i < count; ++i)
            enumeration->setValue(data, enumeration->getValueByName(valueNames[i]));
    }

    void AssetLoader::serializeClass(Object** data, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
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
        for (size_t i = 0; i < count; ++i)
        {
            data[i] = getStringTable().at(stringIndices[i]);
        }
    }

    void AssetLoader::serializeStdArray(StdArray* stdArray, void* data, size_t count)
    {
        const size_t stdArraySize = stdArray->getSize();
        for (size_t i = 0; i < count; ++i)
        {
            void* stdArrayData = static_cast<uint8_t*>(data) + stdArraySize * i;
            Type* elementType = stdArray->getElementType();
            size_t elementCount = stdArray->getElementCount();
            if (elementType->getKind() == Type::Kind::Class)
            {
                Object** objects = new Object*[elementCount];
                serializeType(stdArray->getElementType(), objects, elementCount);
                for (size_t j = 0; j < elementCount; ++j)
                    stdArray->setElementValue(stdArrayData, j, &objects[j]);
                delete[] objects;
            }
            else
            {
                serializeType(elementType, stdArray->getElementPtrByIndex(stdArrayData, 0), elementCount);
            }
        }
    }

    void AssetLoader::serializeStdMap(StdMap* stdMap, void* data, size_t count)
    {
        Type* keyType = stdMap->getKeyType();
        Type* valueType = stdMap->getValueType();
        for (size_t i = 0; i < count; ++i)
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

    void AssetLoader::serializeStdPair(StdPair* stdPair, void* data, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
        {
            void* stdPairData = static_cast<uint8_t*>(data) + stdPair->getSize() * i;
            Type* firstType = stdPair->getFirstType();
            if (firstType->getKind() == Type::Kind::Class)
            {
                Object* object;
                serializeType(firstType, &object);
                stdPair->setFirstValue(stdPairData, &object);
            }
            else
            {
                serializeType(firstType, stdPair->getFirstPtr(stdPairData));
            }

            Type* secondType = stdPair->getSecondType();
            if (secondType->getKind() == Type::Kind::Class)
            {
                Object* object;
                serializeType(secondType, &object);
                stdPair->setSecondValue(stdPairData, &object);
            }
            else
            {
                serializeType(secondType, stdPair->getSecondPtr(stdPairData));
            }
        }
    }

    void AssetLoader::serializeStdSet(StdSet* stdSet, void* data, size_t count)
    {
        Type* elementType = stdSet->getElementType();
        for (size_t i = 0; i < count; ++i)
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

    void AssetLoader::serializeStdTuple(StdTuple* stdTuple, void* data, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
        {
            void* stdTupleData = static_cast<uint8_t*>(data) + stdTuple->getSize() * i;
            std::vector<Type*> tupleTypes = stdTuple->getTypes();
            std::vector<void*> tupleElementPtrs = stdTuple->getElementPtrs(stdTupleData);
            size_t tupleElementCount = stdTuple->getElementCount();
            for (size_t j = 0; j < tupleElementCount; ++j)
            {
                if (tupleTypes[j]->getKind() == Type::Kind::Class)
                {
                    Object* object;
                    serializeType(tupleTypes[j], &object);
                    stdTuple->setElementValue(stdTupleData, j, &object);
                }
                else
                {
                    serializeType(tupleTypes[j], tupleElementPtrs[j]);
                }
            }
        }
    }

    void AssetLoader::serializeStdUnorderedMap(refl::StdUnorderedMap* stdUnorderedMap, void* data, size_t count)
    {
        Type* keyType = stdUnorderedMap->getKeyType();
        Type* valueType = stdUnorderedMap->getValueType();
        for (size_t i = 0; i < count; ++i)
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
        for (size_t i = 0; i < count; ++i)
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
        for (size_t i = 0; i < count; ++i)
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
        for (size_t i = 0; i < count; ++i)
        {
            void* stdVectorData = static_cast<uint8_t*>(data) + stdVector->getSize() * i;
            size_t elementCount;
            serializeArithmetic(&elementCount);
            if (elementCount > 0)
            {
                stdVector->resize(stdVectorData, elementCount);
                if (elementType->getKind() == Type::Kind::Class)
                {
                    Object** objects = new Object * [elementCount];
                    serializeType(elementType, objects, elementCount);
                    for (size_t j = 0; j < elementCount; ++j)
                        stdVector->setElementValue(stdVectorData, j, &objects[j]);
                    delete[] objects;
                }
                else
                {
                    serializeType(elementType, stdVector->getElementPtrByIndex(stdVectorData, 0), elementCount);
                }
            }
        }
    }
}
