#pragma once
#include "Serializer.h"

namespace xxx
{
    class Asset;

    enum AssetSerialFlag : uint32_t
    {
        Big_Endian_Byte_Order               = 1,
        Enum_Serialize_By_Name              = 1 << 2,
    };

    using RawStringTable = std::vector<std::string>;
    using RawImportTable = std::vector<std::pair<uint32_t, Guid>>;
    using RawExportTable = std::vector<std::pair<Guid, osg::ref_ptr<Object>>>;

    class AssetSerializer : public Serializer
    {
        friend class Asset;
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

    private:
        Asset* mAsset;
        std::vector<std::vector<uint8_t>> mObjectBufferTable;
        uint32_t mObjectBufferIndex;
        size_t mObjectBufferPointer;

        //AssetHeader mAssetHeader;
        RawStringTable mRawStringTable;
        RawImportTable mRawImportTable;
        RawExportTable mRawExportTable;
    };
}
