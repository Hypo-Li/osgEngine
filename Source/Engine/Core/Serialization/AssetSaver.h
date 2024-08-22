#pragma once
#include "Serializer.h"
#include "../Object.h"

namespace xxx
{
    class AssetSaver : public Serializer
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

        virtual void serialize(bool& value);
        virtual void serialize(char& value);
        virtual void serialize(wchar_t& value);
        virtual void serialize(int8_t& value);
        virtual void serialize(int16_t& value);
        virtual void serialize(int32_t& value);
        virtual void serialize(int64_t& value);
        virtual void serialize(uint8_t& value);
        virtual void serialize(uint16_t& value);
        virtual void serialize(uint32_t& value);
        virtual void serialize(uint64_t& value);
        virtual void serialize(float& value);
        virtual void serialize(double& value);
        virtual void serialize(std::string& value);

        virtual void serialize(Object*& object) override;

    private:

    };
}
