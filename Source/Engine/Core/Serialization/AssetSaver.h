#pragma once
#include "AssetSerializer.h"
#include "../Object.h"

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

        void serialize(bool* value, size_t count = 1);
        void serialize(char* value, size_t count = 1);
        void serialize(wchar_t* value, size_t count = 1);
        void serialize(int8_t* value, size_t count = 1);
        void serialize(int16_t* value, size_t count = 1);
        void serialize(int32_t* value, size_t count = 1);
        void serialize(int64_t* value, size_t count = 1);
        void serialize(uint8_t* value, size_t count = 1);
        void serialize(uint16_t* value, size_t count = 1);
        void serialize(uint32_t* value, size_t count = 1);
        void serialize(uint64_t* value, size_t count = 1);
        void serialize(float* value, size_t count = 1);
        void serialize(double* value, size_t count = 1);
        void serialize(std::string* value, size_t count = 1);

        virtual void serialize(Object*& object) override;

        bool serializeProperty(refl::Property* property, void* object);

        void serializeType(refl::Type* type, void* data, size_t count = 1);
        void serializeFundamental(refl::Fundamental* type, void* data, size_t count = 1);
        void serializeEnum(refl::Enum* type, void* data, size_t count = 1);
        void serializeStruct(refl::Struct* type, void* data, size_t count = 1);
        void serializeClass(refl::Class* type, void* data, size_t count = 1);
        void serializeSpecial(refl::Special* type, void* data, size_t count = 1);

    private:
        std::unordered_map<Guid, size_t> mObjectGuidMap;
        std::vector<std::pair<std::string, uint32_t>> mStringMap;
        std::vector<uint8_t> mBuffer;
    };
}
