#pragma once
#include "AssetSerializer.h"

namespace xxx
{
    class AssetLoader : public AssetSerializer
    {
    public:
        AssetLoader(Asset* asset) : AssetSerializer(asset)
        {

        }
        virtual ~AssetLoader() = default;

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
        virtual void serialize(std::string* value, size_t count = 1);
    };
}
