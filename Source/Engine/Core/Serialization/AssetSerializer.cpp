#include "AssetSerializer.h"

namespace xxx
{
    using namespace refl;

    static void* newInstanceByType(Type* type)
    {
        Type::Kind kind = type->getKind();
        if (kind == Type::Kind::Fundamental || kind == Type::Kind::Enum)
            return new uint8_t[type->getSize()];
        else if (kind == Type::Kind::Struct)
            return dynamic_cast<Struct*>(type)->newInstance();
        else if (kind == Type::Kind::Class)
            return dynamic_cast<Class*>(type)->newInstance();
        else if (kind == Type::Kind::Special)
            return dynamic_cast<Special*>(type)->newInstance();
        else
            return nullptr;
    }

    static void deleteInstanceByType(Type* type, void* instance)
    {
        Type::Kind kind = type->getKind();
        if (kind == Type::Kind::Fundamental || kind == Type::Kind::Enum)
            delete[](uint8_t*)(instance);
        else if (kind == Type::Kind::Struct)
            dynamic_cast<Struct*>(type)->deleteInstance(instance);
        else if (kind == Type::Kind::Class)
            return dynamic_cast<Class*>(type)->deleteInstance(static_cast<Object*>(instance));
        else if (kind == Type::Kind::Special)
            return dynamic_cast<Special*>(type)->deleteInstance(instance);
    }

    void AssetSerializer::serialize(Object*& object)
    {
        object->preSerialize();
        std::string className;
        Class* clazz = object->getClass();
        if (isSaving())
            className = clazz->getName();
        serialize(&className);
        const auto& properties = clazz->getProperties();
        for (Property* prop : properties)
            serializeProperty(prop, object);
        object->postSerialize();
    }

    void AssetSerializer::serializeProperty(Property* property, void* object)
    {
        std::string propertyName;
        if (isSaving())
            propertyName = property->getName();
        serialize(&propertyName);

        // check property name
        if (isLoading() && propertyName != property->getName())
        {
            uint32_t propertySize;
            serialize(&propertySize);

        }

        void* valuePtr;
        if (property->isValueProperty())
            valuePtr = property->getValuePtr(object);
        else
        {
            valuePtr = new uint8_t[property->getType()->getSize()];
            property->getValue(object, valuePtr);
        }

        serializeType(property->getType(), valuePtr);
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

    void AssetSerializer::serializeFundamental(refl::Fundamental* type, void* data, size_t count)
    {
        if (type == Reflection::BoolType)
            serialize((bool*)(data), count);
        else if (type == Reflection::CharType)
            serialize((char*)(data), count);
        else if (type == Reflection::WCharType)
            serialize((wchar_t*)(data), count);
        else if (type == Reflection::Int8Type)
            serialize((int8_t*)(data), count);
        else if (type == Reflection::Int16Type)
            serialize((int16_t*)(data), count);
        else if (type == Reflection::Int32Type)
            serialize((int32_t*)(data), count);
        else if (type == Reflection::Int64Type)
            serialize((int64_t*)(data), count);
        else if (type == Reflection::Uint8Type)
            serialize((uint8_t*)(data), count);
        else if (type == Reflection::Uint16Type)
            serialize((uint16_t*)(data), count);
        else if (type == Reflection::Uint32Type)
            serialize((uint32_t*)(data), count);
        else if (type == Reflection::Uint64Type)
            serialize((uint64_t*)(data), count);
        else if (type == Reflection::FloatType)
            serialize((float*)(data), count);
        else if (type == Reflection::DoubleType)
            serialize((double*)(data), count);
    }

    template <typename T>
    static void getEnumNames(Enum* type, void* data, std::vector<std::string>& enumNames, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
            enumNames[i] = type->getNameByValue(static_cast<int64_t>(((T*)(data))[i]));
    }

    template <typename T>
    static void setEnumValues(Enum* type, void* data, const std::vector<std::string>& enumNames, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
            ((T*)(data))[i] = static_cast<T>(type->getValueByName(enumNames[i]));
    }

    void AssetSerializer::serializeEnum(Enum* type, void* data, size_t count)
    {
        std::vector<std::string> enumNames(count);
        Type* underlying = type->getUnderlyingType();
        if (isSaving())
        {
            if (underlying == Reflection::Int8Type)
                getEnumNames<int8_t>(type, data, enumNames, count);
            else if (underlying == Reflection::Int16Type)
                getEnumNames<int16_t>(type, data, enumNames, count);
            else if (underlying == Reflection::Int32Type)
                getEnumNames<int32_t>(type, data, enumNames, count);
            else if (underlying == Reflection::Int64Type)
                getEnumNames<int64_t>(type, data, enumNames, count);
            else if (underlying == Reflection::Uint8Type)
                getEnumNames<uint8_t>(type, data, enumNames, count);
            else if (underlying == Reflection::Uint16Type)
                getEnumNames<uint16_t>(type, data, enumNames, count);
            else if (underlying == Reflection::Uint32Type)
                getEnumNames<uint32_t>(type, data, enumNames, count);
            else if (underlying == Reflection::Uint64Type)
                getEnumNames<uint64_t>(type, data, enumNames, count);
            serialize(enumNames.data(), count);
        }
        else
        {
            serialize(enumNames.data(), count);
            if (underlying == Reflection::Int8Type)
                setEnumValues<int8_t>(type, data, enumNames, count);
            else if (underlying == Reflection::Int16Type)
                setEnumValues<int16_t>(type, data, enumNames, count);
            else if (underlying == Reflection::Int32Type)
                setEnumValues<int32_t>(type, data, enumNames, count);
            else if (underlying == Reflection::Int64Type)
                setEnumValues<int64_t>(type, data, enumNames, count);
            else if (underlying == Reflection::Uint8Type)
                setEnumValues<uint8_t>(type, data, enumNames, count);
            else if (underlying == Reflection::Uint16Type)
                setEnumValues<uint16_t>(type, data, enumNames, count);
            else if (underlying == Reflection::Uint32Type)
                setEnumValues<uint32_t>(type, data, enumNames, count);
            else if (underlying == Reflection::Uint64Type)
                setEnumValues<uint64_t>(type, data, enumNames, count);
        }
    }

    void AssetSerializer::serializeStruct(Struct* type, void* data, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
        {
            uint32_t propertyCount;
            if (isSaving())
                propertyCount = type->getProperties().size();
            serialize(&propertyCount);
            for (Property* prop : type->getProperties())
                serializeProperty(prop, data);
        }
    }

    void AssetSerializer::serializeClass(Class* type, void* data, size_t count)
    {

    }

    void AssetSerializer::serializeSpecial(Special* type, void* data, size_t count)
    {
        switch (type->getSpecialType())
        {
        case SpecialType::Std_String:
        {
            serialize((std::string*)(data), count);
            break;
        }
        case SpecialType::Std_Array:
        {
            StdArray* stdArray = dynamic_cast<StdArray*>(type);
            for (size_t i = 0; i < count; ++i)
            {
                void* stdArrayData = (uint8_t*)(data)+stdArray->getSize() * i;
                serializeType(stdArray->getElementType(), stdArray->getElementPtrByIndex(stdArrayData, 0), stdArray->getElementCount());
            }
            break;
        }
        case SpecialType::Std_Map:
        {
            StdMap* stdMap = dynamic_cast<StdMap*>(type);
            if (isSaving())
            {
                for (size_t i = 0; i < count; ++i)
                {
                    void* stdMapData = static_cast<uint8_t*>(data) + stdMap->getSize() * i;
                    std::vector<std::pair<const void*, void*>> keyValuePtrs = stdMap->getKeyValuePtrs(stdMapData);
                    size_t keyValuePairCount = keyValuePtrs.size();
                    std::vector<void*> keyPtrs(keyValuePairCount), valuePtrs(keyValuePairCount);
                    for (size_t j = 0; j < keyValuePairCount; ++j)
                    {
                        keyPtrs[j] = const_cast<void*>(keyValuePtrs[j].first);
                        valuePtrs[j] = keyValuePtrs[j].second;
                    }
                    serialize(&keyValuePairCount);
                    if (keyValuePairCount > 0)
                    {
                        serializeType(stdMap->getKeyType(), keyPtrs[0], keyValuePairCount);
                        serializeType(stdMap->getValueType(), valuePtrs[0], keyValuePairCount);
                    }
                }
            }
            else
            {
                for (size_t i = 0; i < count; ++i)
                {
                    void* stdMapData = static_cast<uint8_t*>(data) + stdMap->getSize() * i;
                    size_t keyValuePairCount;
                    serialize(&keyValuePairCount);
                    std::vector<void*> keyPtrs(keyValuePairCount), valuePtrs(keyValuePairCount);
                    for (size_t j = 0; j < keyValuePairCount; ++j)
                    {
                        keyPtrs[j] = newInstanceByType(stdMap->getKeyType());
                        valuePtrs[j] = newInstanceByType(stdMap->getValueType());
                    }
                    serializeType(stdMap->getKeyType(), keyPtrs[0], keyValuePairCount);
                    serializeType(stdMap->getValueType(), valuePtrs[0], keyValuePairCount);
                    for (size_t j = 0; j < keyValuePairCount; ++j)
                    {
                        stdMap->insertKeyValuePair(stdMapData, keyPtrs[j], valuePtrs[j]);
                        deleteInstanceByType(stdMap->getKeyType(), keyPtrs[j]);
                        deleteInstanceByType(stdMap->getValueType(), valuePtrs[j]);
                    }
                }
            }
            
            break;
        }
        case SpecialType::Std_Pair:
        {
            StdPair* stdPair = dynamic_cast<StdPair*>(type);
            for (size_t i = 0; i < count; ++i)
            {
                void* stdPairData = static_cast<uint8_t*>(data) + stdPair->getSize() * i;
                serializeType(stdPair->getFirstType(), stdPair->getFirstPtr(stdPairData));
                serializeType(stdPair->getSecondType(), stdPair->getSecondPtr(stdPairData));
            }
            break;
        }
        case SpecialType::Std_Set:
        {
            StdSet* stdSet = dynamic_cast<StdSet*>(type);
            for (size_t i = 0; i < count; ++i)
            {
                void* stdSetData = static_cast<uint8_t*>(data) + stdSet->getSize() * i;
                if (isSaving())
                {
                    std::vector<const void*> elementPtrs = stdSet->getElementPtrs(stdSetData);
                    size_t elementCount = elementPtrs.size();
                    serialize(&elementCount);
                    if (elementCount > 0)
                        serializeType(stdSet->getElementType(), const_cast<void*>(elementPtrs[0]), elementCount);
                }
                else
                {
                    size_t elementCount;
                    serialize(&elementCount);
                    if (elementCount > 0)
                    {
                        std::vector<void*> elementPtrs(elementCount);
                        for (size_t j = 0; j < elementCount; ++j)
                            elementPtrs[j] = newInstanceByType(stdSet->getElementType());
                        serializeType(stdSet->getElementType(), elementPtrs[0], elementCount);
                        for (size_t j = 0; j < elementCount; ++j)
                            stdSet->insertElement(stdSetData);
                    }
                }
            }
            break;
        }
        case SpecialType::Std_Tuple:
        {
            StdTuple* stdTuple = dynamic_cast<StdTuple*>(type);
            for (size_t i = 0; i < count; ++i)
            {
                void* currentData = (uint8_t*)(data)+stdTuple->getSize() * i;
                std::vector<Type*> tupleTypes = stdTuple->getTypes();
                std::vector<void*> tupleElementPtrs = stdTuple->getElementPtrs(currentData);
                size_t tupleElementCount = stdTuple->getElementCount();
                for (size_t i = 0; i < tupleElementCount; ++i)
                    serializeType(tupleTypes[i], tupleElementPtrs[i]);
            }
            break;
        }
        case SpecialType::Std_Variant:
        {
            StdVariant* stdVariant = dynamic_cast<StdVariant*>(type);
            for (size_t i = 0; i < count; ++i)
            {
                void* currentData = (uint8_t*)(data)+stdVariant->getSize() * i;
                uint32_t typeIndex = stdVariant->getTypeIndex(currentData);
                Type* currentType = stdVariant->getTypes().at(typeIndex);
                void* valuePtr = stdVariant->getValuePtr(currentData);
                serializeType(currentType, valuePtr);
            }
            break;
        }
        case SpecialType::Std_Vector:
        {
            StdVector* stdVector = dynamic_cast<StdVector*>(type);
            for (size_t i = 0; i < count; ++i)
            {
                void* currentData = (uint8_t*)(data)+stdVector->getSize() * i;
                size_t elementCount = stdVector->getElementCount(currentData);
                serialize(&elementCount);
                if (elementCount > 0)
                    serializeType(stdVector->getElementType(), stdVector->getElementPtrByIndex(currentData, 0), elementCount);
            }
            break;
        }
        default:
            break;
        }
    }
}
