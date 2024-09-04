#include "AssetSerializer.h"

namespace xxx
{
    using namespace refl;

    void AssetSerializer::serialize(Object*& object)
    {
        object->preSerialize();
        Class* clazz = object->getClass();
        if (isSaving())
        {
            const Object* defaultObject = clazz->getDefaultObject();
            std::vector<Property*> serializedProperties;
            Class* baseClass = clazz;
            while (baseClass)
            {
                for (Property* prop : baseClass->getProperties())
                {
                    if (!prop->compare(defaultObject, object))
                        serializedProperties.emplace_back(prop);
                }
                baseClass = baseClass->getBaseClass();
            }
            size_t propertyCount = serializedProperties.size();
            serialize(&propertyCount);

            for (Property* prop : serializedProperties)
            {
                std::string propertyName(prop->getName());
                std::string typeName(prop->getDeclaredType()->getName());
                serialize(&propertyName);
                serialize(&typeName);
                // Save Property Here
            }
        }
        else
        {
            size_t propertyCount;
            serialize(&propertyCount);
            std::vector<Property*> properties = clazz->getProperties();
            for (size_t i = 0; i < propertyCount; ++i)
            {
                std::string propertyName;
                std::string typeName;
                serialize(&propertyName);
                serialize(&typeName);
                auto findResult = std::find_if(properties.begin(), properties.end(),
                    [propertyName](const Property* prop)->bool
                    {
                        return propertyName == prop->getName();
                    }
                );
                if (findResult != properties.end() && (*findResult)->getDeclaredType()->getName() == typeName)
                {
                    Property* prop = *findResult;
                    properties.erase(findResult);
                    
                    // Load Property Here
                }
            }
        }
        
        object->postSerialize();
    }

    void AssetSerializer::serializeProperty(Property* property, void* object)
    {
        void* valuePtr;
        if (property->isMemberProperty())
            valuePtr = property->getValuePtr(object);
        else
        {
            // For AccessorProperty, we need to manually create instances;
            if (property->getDeclaredType()->getKind() == Type::Kind::Class)
            {
                valuePtr = ;
            }
            else
            {
                valuePtr = property->getDeclaredType()->newInstance();
            }
            property->getValue(object, valuePtr);
        }

        serializeType(property->getDeclaredType(), valuePtr);

        if (!property->isMemberProperty())
        {
            if (property->getDeclaredType()->getKind() != Type::Kind::Class)
            {
                property->getDeclaredType()->deleteInstance(valuePtr);
            }
        }
    }

    void AssetSerializer::serializeType(refl::Type* type, void* data, size_t count)
    {
        switch (type->getKind())
        {
        case refl::Type::Kind::Fundamental:
        {
            serializeFundamental(dynamic_cast<Fundamental*>(type), data, count);
            break;
        }
        case refl::Type::Kind::Enum:
        {
            serializeEnum(dynamic_cast<Enum*>(type), data, count);
            break;
        }
        case refl::Type::Kind::Struct:
        {
            serializeStruct(dynamic_cast<Struct*>(type), data, count);
            break;
        }
        case refl::Type::Kind::Class:
        {
            serializeClass(dynamic_cast<Class*>(type), data, count);
            break;
        }
        case refl::Type::Kind::Special:
        {
            serializeSpecial(dynamic_cast<Special*>(type), data, count);
            break;
        }
        default:
            break;
        }
    }

    void AssetSerializer::serializeFundamental(refl::Fundamental* fundamental, void* data, size_t count)
    {
        if (fundamental == Reflection::BoolType)
            serialize((bool*)(data), count);
        else if (fundamental == Reflection::CharType)
            serialize((char*)(data), count);
        else if (fundamental == Reflection::WCharType)
            serialize((wchar_t*)(data), count);
        else if (fundamental == Reflection::Int8Type)
            serialize((int8_t*)(data), count);
        else if (fundamental == Reflection::Int16Type)
            serialize((int16_t*)(data), count);
        else if (fundamental == Reflection::Int32Type)
            serialize((int32_t*)(data), count);
        else if (fundamental == Reflection::Int64Type)
            serialize((int64_t*)(data), count);
        else if (fundamental == Reflection::Uint8Type)
            serialize((uint8_t*)(data), count);
        else if (fundamental == Reflection::Uint16Type)
            serialize((uint16_t*)(data), count);
        else if (fundamental == Reflection::Uint32Type)
            serialize((uint32_t*)(data), count);
        else if (fundamental == Reflection::Uint64Type)
            serialize((uint64_t*)(data), count);
        else if (fundamental == Reflection::FloatType)
            serialize((float*)(data), count);
        else if (fundamental == Reflection::DoubleType)
            serialize((double*)(data), count);
    }

    template <typename T>
    static void getEnumNames(Enum* enumerate, void* data, std::vector<std::string>& enumNames, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
            enumNames[i] = enumerate->getNameByValue(static_cast<int64_t>(((T*)(data))[i]));
    }

    template <typename T>
    static void setEnumValues(Enum* enumerate, void* data, const std::vector<std::string>& enumNames, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
            ((T*)(data))[i] = static_cast<T>(enumerate->getValueByName(enumNames[i]));
    }

    void AssetSerializer::serializeEnum(Enum* enumerate, void* data, size_t count)
    {
        std::vector<std::string> enumNames(count);
        Type* underlying = enumerate->getUnderlyingType();
        if (isSaving())
        {
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
            serialize(enumNames.data(), count);
        }
        else
        {
            serialize(enumNames.data(), count);
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
    }

    void AssetSerializer::serializeStruct(Struct* structure, void* data, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
            for (Property* prop : structure->getProperties())
                serializeType(prop->getDeclaredType(), prop->getValuePtr(data));
    }

    void AssetSerializer::serializeClass(Class* clazz, void* data, size_t count)
    {

    }

    void AssetSerializer::serializeSpecial(Special* special, void* data, size_t count)
    {
        switch (special->getSpecialType())
        {
        case SpecialType::Std_String:
        {
            serialize(static_cast<std::string*>(data), count);
            break;
        }
        case SpecialType::Std_Array:
        {
            serializeStdArray(dynamic_cast<StdArray*>(special), data, count);
            break;
        }
        case SpecialType::Std_Map:
        {
            serializeStdMap(dynamic_cast<StdMap*>(special), data, count);
            break;
        }
        case SpecialType::Std_Pair:
        {
            serializeStdPair(dynamic_cast<StdPair*>(special), data, count);
            break;
        }
        case SpecialType::Std_Set:
        {
            serializeStdSet(dynamic_cast<StdSet*>(special), data, count);
            break;
        }
        case SpecialType::Std_Tuple:
        {
            serializeStdTuple(dynamic_cast<StdTuple*>(special), data, count);
            break;
        }
        case SpecialType::Std_Variant:
        {
            serializeStdVariant(dynamic_cast<StdVariant*>(special), data, count);
            break;
        }
        case SpecialType::Std_Vector:
        {
            serializeStdVector(dynamic_cast<StdVector*>(special), data, count);
            break;
        }
        default:
            break;
        }
    }

    void AssetSerializer::serializeStdArray(refl::StdArray* stdArray, void* data, size_t count)
    {
        const size_t stdArraySize = stdArray->getSize();
        for (size_t i = 0; i < count; ++i)
        {
            void* stdArrayData = static_cast<uint8_t*>(data) + stdArraySize * i;
            serializeType(stdArray->getElementType(), stdArray->getElementPtrByIndex(stdArrayData, 0), stdArray->getElementCount());
        }
    }

    void AssetSerializer::serializeStdMap(refl::StdMap* stdMap, void* data, size_t count)
    {
        Type* keyType = stdMap->getKeyType();
        Type* valueType = stdMap->getValueType();
        if (isSaving())
        {
            for (size_t i = 0; i < count; ++i)
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
        else
        {
            for (size_t i = 0; i < count; ++i)
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
    }

    void AssetSerializer::serializeStdPair(refl::StdPair* stdPair, void* data, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
        {
            void* stdPairData = static_cast<uint8_t*>(data) + stdPair->getSize() * i;
            serializeType(stdPair->getFirstType(), stdPair->getFirstPtr(stdPairData));
            serializeType(stdPair->getSecondType(), stdPair->getSecondPtr(stdPairData));
        }
    }

    void AssetSerializer::serializeStdSet(refl::StdSet* stdSet, void* data, size_t count)
    {
        Type* elementType = stdSet->getElementType();
        const size_t stdSetSize = stdSet->getSize();
        for (size_t i = 0; i < count; ++i)
        {
            void* stdSetData = static_cast<uint8_t*>(data) + stdSetSize * i;
            if (isSaving())
            {
                std::vector<const void*> elementPtrs = stdSet->getElementPtrs(stdSetData);
                size_t elementCount = elementPtrs.size();
                serialize(&elementCount);
                for (size_t j = 0; j < elementCount; ++j)
                    serializeType(elementType, const_cast<void*>(elementPtrs[j]));
            }
            else
            {
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
    }

    void AssetSerializer::serializeStdTuple(refl::StdTuple* stdTuple, void* data, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
        {
            void* stdTupleData = static_cast<uint8_t*>(data) + stdTuple->getSize() * i;
            std::vector<Type*> tupleTypes = stdTuple->getTypes();
            std::vector<void*> tupleElementPtrs = stdTuple->getElementPtrs(stdTupleData);
            size_t tupleElementCount = stdTuple->getElementCount();
            for (size_t i = 0; i < tupleElementCount; ++i)
                serializeType(tupleTypes[i], tupleElementPtrs[i]);
        }
    }

    void AssetSerializer::serializeStdVariant(refl::StdVariant* stdVariant, void* data, size_t count)
    {
        if (isSaving())
        {
            for (size_t i = 0; i < count; ++i)
            {
                void* stdVariantData = static_cast<uint8_t*>(data) + stdVariant->getSize() * i;
                uint32_t typeIndex = stdVariant->getTypeIndex(stdVariantData);
                serialize(&typeIndex);
                Type* variantType = stdVariant->getTypes().at(typeIndex);
                void* valuePtr = stdVariant->getValuePtr(stdVariantData);
                serializeType(variantType, valuePtr);
            }
        }
        else
        {
            for (size_t i = 0; i < count; ++i)
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
    }

    void AssetSerializer::serializeStdVector(refl::StdVector* stdVector, void* data, size_t count)
    {
        Type* elementType = stdVector->getElementType();
        if (isSaving())
        {
            for (size_t i = 0; i < count; ++i)
            {
                void* stdVectorData = static_cast<uint8_t*>(data) + stdVector->getSize() * i;
                size_t elementCount = stdVector->getElementCount(stdVectorData);
                serialize(&elementCount);
                if (elementCount > 0)
                    serializeType(elementType, stdVector->getElementPtrByIndex(stdVectorData, 0), elementCount);
            }
        }
        else
        {
            for (size_t i = 0; i < count; ++i)
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
}
