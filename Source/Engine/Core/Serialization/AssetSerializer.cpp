#include "AssetSerializer.h"
#include "../AssetManager.h"

namespace xxx
{
    using namespace refl;

    uint32_t AssetSerializer::addString(const std::string& str)
    {
        auto findResult = mStringIndexMap.find(str);
        if (findResult == mStringIndexMap.end())
        {
            uint32_t strIndex = mStringTable.size();
            mStringTable.emplace_back(str);
            mStringIndexMap.emplace(mStringTable[strIndex], strIndex);
            return strIndex;
        }
        return findResult->second;
    }

    uint32_t AssetSerializer::addImportItem(uint32_t importItem)
    {
        auto findResult = std::find(mImportTable.begin(), mImportTable.end(), importItem);
        if (findResult == mImportTable.end())
        {
            mImportTable.emplace_back(importItem);
            return mImportTable.size() - 1;
        }
        return findResult - mImportTable.begin();
    }

    void AssetSerializer::serializeFundamental(refl::Fundamental* fundamental, void* data, size_t count)
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

    void AssetSerializer::serializeStructure(Structure* structure, void* data, size_t count)
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            void* structData = static_cast<uint8_t*>(data) + structure->getSize() * i;
            for (Property* prop : structure->getProperties())
                serializeType(prop->getDeclaredType(), prop->getValuePtr(structData));
        }
    }
}
