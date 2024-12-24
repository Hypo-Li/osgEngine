#include "AssetLoader.h"
#include "../Asset.h"
#include "../AssetManager.h"

namespace xxx
{
    using namespace refl;

    AssetLoader::AssetLoader(Asset* asset) : AssetSerializer(asset) {}

    void AssetLoader::serializeEnumeration(Enumeration* enumeration, void* data, size_t count)
    {
        std::vector<std::string> valueNames(count);
        serializeStdString(nullptr, valueNames.data(), count);
        for (size_t i = 0; i < count; ++i)
            enumeration->setValue(data, enumeration->getValueByName(valueNames[i]));
    }

    void AssetLoader::serializeClass(Class* clazz, void* data, size_t count)
    {
        Object** objects = static_cast<Object**>(data);
        for (size_t i = 0; i < count; ++i)
        {
            Object*& object = objects[i];
            int32_t objectIndex;
            if (getCurrentObjectBuffer())
                serializeArithmetic(&objectIndex);
            else
                objectIndex = -1;

            if (objectIndex == 0)
            {
                object = nullptr;
            }
            else if (objectIndex > 0) // Imported object
            {
                uint32_t importIndex = objectIndex - 1;
                const std::string& path = getString(getImportItem(importIndex));
                Asset* asset = AssetManager::get().getAsset(path);
                if (asset)
                {
                    object = asset->getRootObjectSafety();
                }
                else
                {
                    //LOG_ERROR("Cannot find asset: {}", path);
                    object = nullptr;
                }
            }
            else // Exported object
            {
                uint32_t exportIndex = -objectIndex - 1;
                auto objectFindResult = mObjectsTemp.find(exportIndex);
                if (objectFindResult == mObjectsTemp.end())
                {
                    pushObjectBuffer(exportIndex);
                    std::string className;
                    serializeStdString(nullptr, &className);
                    Class* clazz = Reflection::getClass(className);
                    object = static_cast<Object*>(clazz->newInstance());
                    mObjectsTemp.emplace(exportIndex, object);
                    serializeObject(object);
                    popObjectBuffer();
                }
                else
                {
                    object = objectFindResult->second;
                }
            }
        }
    }

    void AssetLoader::serializeStdArray(StdArray* stdArray, void* data, size_t count)
    {
        Type* elementType = stdArray->getElementType();
        size_t elementCount = stdArray->getElementCount();
        for (size_t i = 0; i < count; ++i)
        {
            void* stdArrayData = static_cast<uint8_t*>(data) + stdArray->getSize() * i;
            if (elementType->getKind() == Type::Kind::Class)
            {
                Object** objects = new Object * [elementCount];
                serializeClass(dynamic_cast<Class*>(elementType), objects, elementCount);
                for (size_t j = 0; j < elementCount; ++j)
                    stdArray->setElement(stdArrayData, j, &objects[j]);
                delete[] objects;
            }
            else
            {
                serializeType(elementType, stdArray->getElementPtr(stdArrayData, 0), elementCount);
            }
        }
    }

    void AssetLoader::serializeStdList(StdList* stdList, void* data, size_t count)
    {
        Type* elementType = stdList->getElementType();
        const bool elementTypeIsClass = elementType->getKind() == Type::Kind::Class;
        const size_t elementSize = elementTypeIsClass ? sizeof(Object*) : elementType->getSize();
        for (size_t i = 0; i < count; ++i)
        {
            void* stdListData = static_cast<uint8_t*>(data) + stdList->getSize() * i;
            size_t elementCount;
            serializeArithmetic(&elementCount);

            if (elementCount == 0)
                continue;

            void* elements = elementTypeIsClass ?
                new Object * [elementCount] :
                elementType->newInstances(elementCount);
            serializeType(elementType, elements, elementCount);

            for (size_t j = 0; j < elementCount; ++j)
            {
                void* element = static_cast<uint8_t*>(elements) + j * elementSize;
                stdList->addElement(stdListData, element);
            }

            if (elementTypeIsClass)
                delete[] static_cast<Object**>(elements);
            else
                elementType->deleteInstances(elements);
        }
    }

    void AssetLoader::serializeStdMap(StdMap* stdMap, void* data, size_t count)
    {
        Type* keyType = stdMap->getKeyType();
        Type* valueType = stdMap->getValueType();
        const bool keyTypeIsClass = keyType->getKind() == Type::Kind::Class;
        const bool valueTypeIsClass = valueType->getKind() == Type::Kind::Class;
        const size_t keySize = keyTypeIsClass ? sizeof(Object*) : keyType->getSize();
        const size_t valueSize = valueTypeIsClass ? sizeof(Object*) : valueType->getSize();
        for (size_t i = 0; i < count; ++i)
        {
            void* stdMapData = static_cast<uint8_t*>(data) + stdMap->getSize() * i;
            size_t keyValuePairCount;
            serializeArithmetic(&keyValuePairCount);

            if (keyValuePairCount == 0)
                continue;

            void* keys = keyTypeIsClass ?
                new Object * [keyValuePairCount] :
                keyType->newInstances(keyValuePairCount);
            void* values = valueTypeIsClass ?
                new Object * [keyValuePairCount] :
                valueType->newInstances(keyValuePairCount);
            serializeType(keyType, keys, keyValuePairCount);
            serializeType(valueType, values, keyValuePairCount);

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
        Type* firstType = stdPair->getFirstType();
        Type* secondType = stdPair->getSecondType();
        for (size_t i = 0; i < count; ++i)
        {
            void* stdPairData = static_cast<uint8_t*>(data) + stdPair->getSize() * i;

            if (firstType->getKind() == Type::Kind::Class)
            {
                Object* object;
                serializeClass(dynamic_cast<Class*>(firstType), &object);
                stdPair->setFirstValue(stdPairData, &object);
            }
            else
            {
                serializeType(firstType, stdPair->getFirstPtr(stdPairData));
            }

            if (secondType->getKind() == Type::Kind::Class)
            {
                Object* object;
                serializeClass(dynamic_cast<Class*>(secondType), &object);
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
        const bool elementTypeIsClass = elementType->getKind() == Type::Kind::Class;
        const size_t elementSize = elementTypeIsClass ? sizeof(Object*) : elementType->getSize();
        for (size_t i = 0; i < count; ++i)
        {
            void* stdSetData = static_cast<uint8_t*>(data) + stdSet->getSize() * i;
            size_t elementCount;
            serializeArithmetic(&elementCount);

            if (elementCount == 0)
                continue;

            void* elements = elementTypeIsClass ?
                new Object * [elementCount] :
                elementType->newInstances(elementCount);
            serializeType(elementType, elements, elementCount);

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

    void AssetLoader::serializeStdString(StdString* stdString, void* data, size_t count)
    {
        std::vector<uint32_t> strIndices(count);
        serializeArithmetic(strIndices.data(), count);
        for (size_t i = 0; i < count; ++i)
            static_cast<std::string*>(data)[i] = getString(strIndices[i]);
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
        std::vector<Type*> types = stdVariant->getTypes();
        for (size_t i = 0; i < count; ++i)
        {
            void* stdVariantData = static_cast<uint8_t*>(data) + stdVariant->getSize() * i;
            uint32_t typeIndex;
            serializeArithmetic(&typeIndex);
            Type* variantType = types.at(typeIndex);
            if (variantType->getKind() == Type::Kind::Class)
            {
                Object* object;
                serializeClass(dynamic_cast<Class*>(variantType), &object);
                stdVariant->setValue(stdVariantData, typeIndex, &object);
            }
            else
            {
                void* valuePtr = variantType->newInstance();
                serializeType(variantType, valuePtr);
                stdVariant->setValue(stdVariantData, typeIndex, valuePtr);
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

            if (elementCount == 0)
                continue;

            stdVector->resize(stdVectorData, elementCount);
            if (elementType->getKind() == Type::Kind::Class)
            {
                Object** objects = new Object * [elementCount];
                serializeClass(dynamic_cast<Class*>(elementType), objects, elementCount);
                for (size_t j = 0; j < elementCount; ++j)
                    stdVector->setElement(stdVectorData, j, &objects[j]);
                delete[] objects;
            }
            else
            {
                serializeType(elementType, stdVector->getElementPtr(stdVectorData, 0), elementCount);
            }
        }
    }

    void AssetLoader::serializeBinary(void* data, size_t size)
    {
        getCurrentObjectBuffer()->read(data, size);
    }

    void AssetLoader::serializeObject(Object* object)
    {
        ObjectBuffer* objectBuffer = getCurrentObjectBuffer();

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
            serializeStdString(nullptr, &propertyName);
            serializeArithmetic(&propertySize);

            auto findResult = std::find_if(properties.begin(), properties.end(),
                [propertyName](const Property* prop)->bool { return propertyName == prop->getName(); }
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
                objectBuffer->seek(objectBuffer->tell() + propertySize);
            }
        }

        object->postLoad();
    }
}
