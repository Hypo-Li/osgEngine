#pragma once
#include "Serializer.h"
#include <stack>

namespace xxx
{
    class Asset;

    enum AssetSerialFlag : uint32_t
    {
        Big_Endian_Byte_Order               = 1,
        Enum_Serialize_By_Name              = 1 << 2,
    };

    class AssetSerializer : public Serializer
    {
        friend class Asset;
    public:
        AssetSerializer(Asset* asset) : mAsset(asset)
        {
            pushObjectBufferIndex(createNewObjectBuffer());
        }
        virtual ~AssetSerializer() = 0;
        virtual bool isLoading() const = 0;
        virtual bool isSaving() const = 0;

        virtual void serializeObject(Object*& object) override;

    protected:
        inline void serialize(bool* data, size_t count = 1)
        {
            serializeBinary(data, sizeof(bool) * count);
        }

        inline void serialize(char* data, size_t count = 1)
        {
            serializeBinary(data, sizeof(char) * count);
        }

        inline void serialize(wchar_t* data, size_t count = 1)
        {
            serializeBinary(data, sizeof(wchar_t) * count);
        }

        inline void serialize(int8_t* data, size_t count = 1)
        {
            serializeBinary(data, sizeof(int8_t) * count);
        }

        inline void serialize(int16_t* data, size_t count = 1)
        {
            serializeBinary(data, sizeof(int16_t) * count);
        }

        inline void serialize(int32_t* data, size_t count = 1)
        {
            serializeBinary(data, sizeof(int32_t) * count);
        }

        inline void serialize(int64_t* data, size_t count = 1)
        {
            serializeBinary(data, sizeof(int64_t) * count);
        }

        inline void serialize(uint8_t* data, size_t count = 1)
        {
            serializeBinary(data, sizeof(uint8_t) * count);
        }

        inline void serialize(uint16_t* data, size_t count = 1)
        {
            serializeBinary(data, sizeof(uint16_t) * count);
        }

        inline void serialize(uint32_t* data, size_t count = 1)
        {
            serializeBinary(data, sizeof(uint32_t) * count);
        }

        inline void serialize(uint64_t* data, size_t count = 1)
        {
            serializeBinary(data, sizeof(uint64_t) * count);
        }

        inline void serialize(float* data, size_t count = 1)
        {
            serializeBinary(data, sizeof(float) * count);
        }

        inline void serialize(double* data, size_t count = 1)
        {
            serializeBinary(data, sizeof(double) * count);
        }

        virtual void serializeBinary(void* data, size_t count) = 0;

        // serialize types
        void serializeType(refl::Type* type, void* data, size_t count = 1);
        void serializeFundamental(refl::Fundamental* fundamental, void* data, size_t count = 1);
        void serializeEnum(refl::Enum* enumerate, void* data, size_t count = 1);
        void serializeStruct(refl::Struct* structure, void* data, size_t count = 1);
        void serializeClass(refl::Class* clazz, void* data, size_t count = 1);
        void serializeSpecial(refl::Special* special, void* data, size_t count = 1);

        // serialize specials
        void serializeStdString(std::string* data, size_t count = 1);
        void serializeStdArray(refl::StdArray* stdArray, void* data, size_t count = 1);
        void serializeStdMap(refl::StdMap* stdMap, void* data, size_t count = 1);
        void serializeStdPair(refl::StdPair* stdPair, void* data, size_t count = 1);
        void serializeStdSet(refl::StdSet* stdSet, void* data, size_t count = 1);
        void serializeStdTuple(refl::StdTuple* stdTuple, void* data, size_t count = 1);
        void serializeStdVariant(refl::StdVariant* stdVariant, void* data, size_t count = 1);
        void serializeStdVector(refl::StdVector* stdVector, void* data, size_t count = 1);

        inline uint32_t createNewObjectBuffer()
        {
            uint32_t index = mObjectBufferTable.size();
            mObjectBufferTable.emplace_back(std::vector<uint8_t>{}, 0);
            return index;
        }

        inline void pushObjectBufferIndex(uint32_t index)
        {
            mObjectBufferIndexStack.push(index);
        }

        inline uint32_t popObjectBufferIndex()
        {
            mObjectBufferIndexStack.pop();
        }

        inline uint32_t tell() const
        {
            uint32_t objectBufferIndex = mObjectBufferIndexStack.top();
            if (objectBufferIndex >= mObjectBufferTable.size())
                return uint32_t(-1);
            return mObjectBufferTable[objectBufferIndex].second;
        }

        inline void seek(uint32_t pos)
        {
            uint32_t objectBufferIndex = mObjectBufferIndexStack.top();
            if (objectBufferIndex >= mObjectBufferTable.size())
                return;
            mObjectBufferTable[objectBufferIndex].second = pos;
        }

        inline uint32_t getStringTableIndex(const std::string& str)
        {
            auto findResult = std::find(mRawStringTable.begin(), mRawStringTable.end(), str);
            if (findResult == mRawStringTable.end())
            {
                mRawStringTable.emplace_back(str);
                return mRawStringTable.size() - 1;
            }
            else
            {
                return findResult - mRawStringTable.begin();
            }
        }

        inline const std::string& getStringTableStr(uint32_t index)
        {
            return mRawStringTable.at(index);
        }

    protected:
        Asset* mAsset;

        using RawStringTable = std::vector<std::string>;
        using RawImportTable = std::vector<std::pair<uint32_t, Guid>>;
        using RawExportTable = std::vector<std::pair<Guid, osg::ref_ptr<Object>>>;
        RawStringTable mRawStringTable;
        RawImportTable mRawImportTable;
        RawExportTable mRawExportTable;

    private:
        std::vector<std::pair<std::vector<uint8_t>, uint32_t>> mObjectBufferTable;
        std::stack<uint32_t> mObjectBufferIndexStack;
    };
}
