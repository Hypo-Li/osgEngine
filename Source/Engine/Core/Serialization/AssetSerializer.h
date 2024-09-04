#pragma once
#include "Serializer.h"
#include "../Asset.h"

namespace xxx
{
    class AssetSerializer : public Serializer
    {
    public:
        AssetSerializer(Asset* asset) : mAsset(asset) {}
        virtual ~AssetSerializer() = 0;
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
        void serializeFundamental(refl::Fundamental* fundamental, void* data, size_t count = 1);
        void serializeEnum(refl::Enum* enumerate, void* data, size_t count = 1);
        void serializeStruct(refl::Struct* structure, void* data, size_t count = 1);
        void serializeClass(refl::Class* clazz, void* data, size_t count = 1);
        void serializeSpecial(refl::Special* special, void* data, size_t count = 1);

        void serializeStdArray(refl::StdArray* stdArray, void* data, size_t count = 1);
        void serializeStdMap(refl::StdMap* stdMap, void* data, size_t count = 1);
        void serializeStdPair(refl::StdPair* stdPair, void* data, size_t count = 1);
        void serializeStdSet(refl::StdSet* stdSet, void* data, size_t count = 1);
        void serializeStdTuple(refl::StdTuple* stdTuple, void* data, size_t count = 1);
        void serializeStdVariant(refl::StdVariant* stdVariant, void* data, size_t count = 1);
        void serializeStdVector(refl::StdVector* stdVector, void* data, size_t count = 1);

    protected:
        Asset* mAsset;
        std::vector<uint8_t> mBuffer;
    };
}
