#pragma once
#include "AssetSerializer.h"
#include "../Asset.h"

namespace xxx
{
    class AssetSaver : public AssetSerializer
    {
    public:
        AssetSaver();
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
            std::vector<uint32_t> stringIndices(count);
            for (size_t i = 0; i < count; ++i)
            {
                auto findResult = std::find(mAsset->mStringTable.begin(), mAsset->mStringTable.end(), value[i]);
                if (findResult == mAsset->mStringTable.end())
                {
                    mAsset->mStringTable.emplace_back(value[i]);
                    stringIndices[i] = mAsset->mStringTable.size() - 1;
                }
                else
                {
                    stringIndices[i] = findResult - mAsset->mStringTable.begin();
                }
            }
            serialize(stringIndices.data(), count);
        }

    };
}
