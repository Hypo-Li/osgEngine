#include "AssetSerializer.h"
#include "../AssetManager.h"

namespace xxx
{
    using namespace refl;

    void AssetSerializer::serializeType(refl::Type* type, void* data, uint32_t count)
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

    void AssetSerializer::serializeFundamental(refl::Fundamental* fundamental, void* data, uint32_t count)
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

    void AssetSerializer::serializeStruct(Struct* structure, void* data, uint32_t count)
    {
        for (uint32_t i = 0; i < count; ++i)
            for (Property* prop : structure->getProperties())
                serializeType(prop->getDeclaredType(), prop->getValuePtr(data));
    }

    void AssetSerializer::serializeSpecial(Special* special, void* data, uint32_t count)
    {
        switch (special->getSpecialType())
        {
        case SpecialType::Std_String:
        {
            serializeStdString(static_cast<std::string*>(data), count);
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

    void AssetSerializer::serializeStdArray(StdArray* stdArray, void* data, uint32_t count)
    {
        const size_t stdArraySize = stdArray->getSize();
        for (uint32_t i = 0; i < count; ++i)
        {
            void* stdArrayData = static_cast<uint8_t*>(data) + stdArraySize * i;
            serializeType(stdArray->getElementType(), stdArray->getElementPtrByIndex(stdArrayData, 0), stdArray->getElementCount());
        }
    }

    void AssetSerializer::serializeStdPair(StdPair* stdPair, void* data, uint32_t count)
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            void* stdPairData = static_cast<uint8_t*>(data) + stdPair->getSize() * i;
            serializeType(stdPair->getFirstType(), stdPair->getFirstPtr(stdPairData));
            serializeType(stdPair->getSecondType(), stdPair->getSecondPtr(stdPairData));
        }
    }

    void AssetSerializer::serializeStdTuple(StdTuple* stdTuple, void* data, uint32_t count)
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            void* stdTupleData = static_cast<uint8_t*>(data) + stdTuple->getSize() * i;
            std::vector<Type*> tupleTypes = stdTuple->getTypes();
            std::vector<void*> tupleElementPtrs = stdTuple->getElementPtrs(stdTupleData);
            size_t tupleElementCount = stdTuple->getElementCount();
            for (size_t i = 0; i < tupleElementCount; ++i)
                serializeType(tupleTypes[i], tupleElementPtrs[i]);
        }
    }
}
