#pragma once
#include "AssetSerializer.h"

namespace xxx
{
    class AssetSaver : public AssetSerializer
    {
    public:
        virtual ~AssetSaver() = default;

        virtual bool isLoading() const override
        {
            return false;
        }

        virtual bool isSaving() const override
        {
            return true;
        }

        virtual void serialize(bool* value, size_t count = 1);
        virtual void serialize(char* value, size_t count = 1);
        virtual void serialize(wchar_t* value, size_t count = 1);
        virtual void serialize(int8_t* value, size_t count = 1);
        virtual void serialize(int16_t* value, size_t count = 1);
        virtual void serialize(int32_t* value, size_t count = 1);
        virtual void serialize(int64_t* value, size_t count = 1);
        virtual void serialize(uint8_t* value, size_t count = 1);
        virtual void serialize(uint16_t* value, size_t count = 1);
        virtual void serialize(uint32_t* value, size_t count = 1);
        virtual void serialize(uint64_t* value, size_t count = 1);
        virtual void serialize(float* value, size_t count = 1);
        virtual void serialize(double* value, size_t count = 1);
        virtual void serialize(std::string* value, size_t count = 1) override
        {
            uint32_t* stringOffsets = new uint32_t[count];
            for (size_t i = 0; i < count; ++i)
            {
                auto findResult = mStringTable.find(value[i]);
                if (findResult == mStringTable.end())
                {
                    mStringTable.emplace(value[i], mStringOffset);
                    stringOffsets[i] = mStringOffset;
                    mStringOffset += sizeof(uint32_t) + value[i].size();
                }
                else
                {
                    stringOffsets[i] = findResult->second;
                }
            }
            serialize(stringOffsets, count);
            delete[] stringOffsets;
        }

    };
}
