#include "AssetSerializer.h"
#include "../AssetManager.h"

namespace xxx
{
    using namespace refl;

    uint32_t AssetSerializer::addString(const std::string& str)
    {
        auto findResult = std::find(mStringTable.begin(), mStringTable.end(), str);
        if (findResult == mStringTable.end())
        {
            mStringTable.emplace_back(str);
            return mStringTable.size() - 1;
        }
        else
        {
            return findResult - mStringTable.begin();
        }
    }

    int32_t AssetSerializer::getIndexOfObject(Object* object)
    {
        int32_t index = 0;
        if (object == nullptr)
            index = 0;
        else if (object->getAsset() != mAsset && object->getAsset() != nullptr)
        {
            // imported object
            auto findResult = std::find(mImportTable.begin(), mImportTable.end(), object->getGuid());

            if (findResult == mImportTable.end())
            {
                // if no saved
                mImportTable.emplace_back(object->getGuid());
                index = mImportTable.size() - 1;
            }
            else
            {
                // if saved
                index = findResult - mImportTable.begin();
            }

            index = index + 1;
        }
        else
        {
            // exported object
            auto findResult = std::find_if(
                mExportTable.begin(), mExportTable.end(),
                [object](ExportItem& item)
                {
                return item.guid == object->getGuid();
                }
            );

            if (findResult == mExportTable.end())
            {
                // if no saved
                index = mExportTable.size();
                std::string className(object->getClass()->getName());
                mExportTable.emplace_back(object->getGuid(), addString(className));

                pushObjectBufferIndex(createNewObjectBuffer());
                serializeObject(object);
                popObjectBufferIndex();
            }
            else
            {
                // if saved
                index = findResult - mExportTable.begin();
            }

            index = -(index + 1);
        }
        return index;
    }

    Object* AssetSerializer::getObjectByIndex(int32_t index)
    {
        Object* object;
        if (index == 0)
            object = nullptr;
        else if (index > 0)
        {
            // imported object
            index = index - 1;
            Guid guid = mImportTable[index];

            Asset* asset = AssetManager::get().getAsset(guid);
            object = asset->getRootObject();
        }
        else
        {
            // exported object
            index = -index - 1;
            if (mExportTable[index].tempObject == nullptr)
            {
                // if no loaded
                Guid guid = mExportTable[index].guid;
                Class* clazz = Reflection::getClass(getString(mExportTable[index].className));
                object = static_cast<Object*>(clazz->newInstance());
                mExportTable[index].tempObject = object;

                // object->mGuid = guid;
                pushObjectBufferIndex(index);
                serializeObject(object);
                popObjectBufferIndex();
            }
            else
            {
                // if loaded
                object = mExportTable[index].tempObject;
            }
        }
        return object;
    }

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
            serializeClass(static_cast<Object**>(data), count);
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
            serializeArithmetic((bool*)(data), count);
        else if (fundamental == Reflection::CharType)
            serializeArithmetic((char*)(data), count);
        else if (fundamental == Reflection::WCharType)
            serializeArithmetic((wchar_t*)(data), count);
        else if (fundamental == Reflection::Int8Type)
            serializeArithmetic((int8_t*)(data), count);
        else if (fundamental == Reflection::Int16Type)
            serializeArithmetic((int16_t*)(data), count);
        else if (fundamental == Reflection::Int32Type)
            serializeArithmetic((int32_t*)(data), count);
        else if (fundamental == Reflection::Int64Type)
            serializeArithmetic((int64_t*)(data), count);
        else if (fundamental == Reflection::Uint8Type)
            serializeArithmetic((uint8_t*)(data), count);
        else if (fundamental == Reflection::Uint16Type)
            serializeArithmetic((uint16_t*)(data), count);
        else if (fundamental == Reflection::Uint32Type)
            serializeArithmetic((uint32_t*)(data), count);
        else if (fundamental == Reflection::Uint64Type)
            serializeArithmetic((uint64_t*)(data), count);
        else if (fundamental == Reflection::FloatType)
            serializeArithmetic((float*)(data), count);
        else if (fundamental == Reflection::DoubleType)
            serializeArithmetic((double*)(data), count);
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
