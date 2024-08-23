#include "AssetSaver.h"

namespace xxx
{
    using namespace refl;
    void AssetSaver::serialize(Object*& object)
    {
        Class* clazz = object->getClass();
        const auto& properties = clazz->getProperties();
        for (Property* prop : properties)
            serializeProperty(prop, object);
    }

    void AssetSaver::serializeType(Type* type, void* data, size_t count)
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

    void AssetSaver::serializeFundamental(Fundamental* type, void* data, size_t count)
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
        else
        {
            // LOG_ERROR: Unserializable Fundamental
        }
    }

    template <typename T>
    void getEnumValueNames(std::vector<std::string>& enumValueNames, void* data, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
            enumValueNames[i] = type->getNameByValue(static_cast<int64_t>(((T*)(data))[i]));
    }

    void AssetSaver::serializeEnum(Enum* type, void* data, size_t count)
    {
        std::vector<std::string> enumValueNames(count);
        Type* underlying = type->getUnderlyingType();
        if (underlying == Reflection::Int8Type)
            getEnumValueNames<int8_t>(enumValueNames, data, count);
        else if (underlying == Reflection::Int16Type)
            getEnumValueNames<int16_t>(enumValueNames, data, count);
        else if (underlying == Reflection::Int32Type)
            getEnumValueNames<int32_t>(enumValueNames, data, count);
        else if (underlying == Reflection::Int64Type)
            getEnumValueNames<int64_t>(enumValueNames, data, count);
        else if (underlying == Reflection::Uint8Type)
            getEnumValueNames<uint8_t>(enumValueNames, data, count);
        else if (underlying == Reflection::Uint16Type)
            getEnumValueNames<uint16_t>(enumValueNames, data, count);
        else if (underlying == Reflection::Uint32Type)
            getEnumValueNames<uint32_t>(enumValueNames, data, count);
        else if (underlying == Reflection::Uint64Type)
            getEnumValueNames<uint64_t>(enumValueNames, data, count);
        serialize(enumValueNames.data(), count);
    }

    void AssetSaver::serializeStruct(Struct* type, void* data, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
        {
            uint32_t propertyCount = type->getProperties().size();
            serialize(&propertyCount);
            for (Property* prop : type->getProperties())
                serializeProperty(prop, data);
        }
    }

    void AssetSaver::serializeSpecial(Special* type, void* data, size_t count)
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
                void* currentData = (uint8_t*)(data) + stdArray->getSize() * i;
                serializeType(stdArray->getElementType(), stdArray->getElementPtrByIndex(currentData, 0), stdArray->getElementCount());
            }
            break;
        }
        case SpecialType::Std_Map:
        {
            StdMap* stdMap = dynamic_cast<StdMap*>(type);
            for (size_t i = 0; i < count; ++i)
            {
                void* currentData = (uint8_t*)(data) + stdMap->getSize() * i;
                std::vector<std::pair<const void*, void*>> keyValuePtrs = stdMap->getKeyValuePtrs(currentData);
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
            break;
        }
        case SpecialType::Std_Pair:
        {
            StdPair* stdPair = dynamic_cast<StdPair*>(type);
            for (size_t i = 0; i < count; ++i)
            {
                void* currentData = (uint8_t*)(data) + stdPair->getSize() * i;
                serializeType(stdPair->getFirstType(), stdPair->getFirstPtr(currentData));
                serializeType(stdPair->getSecondType(), stdPair->getSecondPtr(currentData));
            }
            break;
        }
        case SpecialType::Std_Set:
        {
            StdSet* stdSet = dynamic_cast<StdSet*>(type);
            for (size_t i = 0; i < count; ++i)
            {
                void* currentData = (uint8_t*)(data) + stdSet->getSize() * i;
                std::vector<const void*> elementPtrs = stdSet->getElementPtrs(currentData);
                size_t elementCount = elementPtrs.size();
                serialize(&elementCount);
                if (elementCount > 0)
                    serializeType(stdSet->getElementType(), const_cast<void*>(elementPtrs[0]), elementCount);
            }
            break;
        }
        case SpecialType::Std_Tuple:
        {
            StdTuple* stdTuple = dynamic_cast<StdTuple*>(type);
            for (size_t i = 0; i < count; ++i)
            {
                void* currentData = (uint8_t*)(data) + stdTuple->getSize() * i;
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
                void* currentData = (uint8_t*)(data) + stdVariant->getSize() * i;
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
                void* currentData = (uint8_t*)(data) + stdVector->getSize() * i;
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

    bool AssetSaver::serializeProperty(Property* property, void* object)
    {
        std::string propertyName(property->getName());
        serialize(&propertyName);

        void* valuePtr;
        if (property->isValueProperty())
            valuePtr = property->getValuePtr(object);
        else
        {
            valuePtr = new uint8_t[property->getType()->getSize()];
            property->getValue(object, valuePtr);
        }

        serializeType(property->getType(), valuePtr);
        return true;
    }
}
