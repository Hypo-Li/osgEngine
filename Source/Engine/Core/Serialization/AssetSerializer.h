#pragma once
#include "Serializer.h"
#include "../Asset.h"

namespace xxx
{
    class AssetSerializer : public Serializer
    {
    public:
        virtual bool isLoading() const = 0;
        virtual bool isSaving() const = 0;

        virtual void serialize(Object*& object) override;

    protected:
        virtual void serialize(bool* value, size_t count = 1) = 0;
        virtual void serialize(char* value, size_t count = 1) = 0;
        virtual void serialize(wchar_t* value, size_t count = 1) = 0;
        virtual void serialize(int8_t* value, size_t count = 1) = 0;
        virtual void serialize(int16_t* value, size_t count = 1) = 0;
        virtual void serialize(int32_t* value, size_t count = 1) = 0;
        virtual void serialize(int64_t* value, size_t count = 1) = 0;
        virtual void serialize(uint8_t* value, size_t count = 1) = 0;
        virtual void serialize(uint16_t* value, size_t count = 1) = 0;
        virtual void serialize(uint32_t* value, size_t count = 1) = 0;
        virtual void serialize(uint64_t* value, size_t count = 1) = 0;
        virtual void serialize(float* value, size_t count = 1) = 0;
        virtual void serialize(double* value, size_t count = 1) = 0;
        virtual void serialize(std::string* value, size_t count = 1) = 0;

        void serializeProperty(refl::Property* property, void* object);
        void serializeType(refl::Type* type, void* data, size_t count = 1);
        void serializeFundamental(refl::Fundamental* type, void* data, size_t count = 1);
        void serializeEnum(refl::Enum* type, void* data, size_t count = 1);
        void serializeStruct(refl::Struct* type, void* data, size_t count = 1);
        void serializeClass(refl::Class* type, void* data, size_t count = 1);
        void serializeSpecial(refl::Special* type, void* data, size_t count = 1);

    protected:
        Asset* mAsset;
        std::vector<uint8_t> mBuffer;
    };
}
