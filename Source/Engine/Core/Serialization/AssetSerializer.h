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

    class ObjectBuffer
    {
    public:
        ObjectBuffer(uint32_t bufferSize = 0, uint32_t pointer = 0) :
            mBuffer(bufferSize), mPointer(pointer) {}

        inline uint32_t tell() const
        {
            return mPointer;
        }

        inline bool seek(uint32_t pos)
        {
            if (pos <= mBuffer.size())
            {
                mPointer = pos;
                return true;
            }
            return false;
        }

        inline uint32_t getSize() const
        {
            return mBuffer.size();
        }

        inline const void* getData() const
        {
            return mBuffer.data();
        }

        inline void writeData(const void* data, uint32_t size)
        {
            uint32_t newPointer = mPointer + size;
            if (newPointer > mBuffer.size())
                mBuffer.resize(newPointer);
            std::memcpy(mBuffer.data() + mPointer, data, size);
            mPointer = newPointer;
        }

        inline void readData(void* data, uint32_t size)
        {
            std::memcpy(data, mBuffer.data() + mPointer, size);
            mPointer += size;
        }

    protected:
        std::vector<uint8_t> mBuffer;
        uint32_t mPointer;
    };

    struct ImportItem
    {
        Guid objectGuid;
        uint32_t pathStrIndex;
    };

    struct ExportItem
    {
        Guid objectGuid;
        uint32_t classNameStrIndex;
    };

    struct AssetHeader;
    class AssetSerializer : public Serializer
    {
    public:
        AssetSerializer(Asset* asset) : mAsset(asset) {}
        virtual ~AssetSerializer() = default;

        // serialize from root object
        virtual void serialize(Object** object) override
        {
            serializeClass(object);
        }

        void fillAssetHeader(AssetHeader& header);

        uint32_t addString(const std::string& str);

        uint32_t addImportItem(const ImportItem& importItem);

        uint32_t addExportItem(const ExportItem& exportItem);

        uint32_t createNewObjectBuffer(uint32_t bufferSize = 0);

        inline const std::vector<std::string>& getStringTable() const
        {
            return mStringTable;
        }

        inline const std::vector<ImportItem>& getImportTable() const
        {
            return mImportTable;
        }

        inline const std::vector<ExportItem>& getExportTable() const
        {
            return mExportTable;
        }

        inline const std::vector<ObjectBuffer>& getObjectBufferTable() const
        {
            return mObjectBufferTable;
        }

    protected:
        // using in AssetSaver
        int32_t getIndexOfObject(Object* object);

        // using in AssetLoader
        Object* getObjectByIndex(int32_t index);

        inline void pushObjectBufferIndex(uint32_t index)
        {
            mObjectBufferIndexStack.push(index);
        }

        inline void popObjectBufferIndex()
        {
            mObjectBufferIndexStack.pop();
        }

        inline uint32_t tell() const
        {
            return getCurrentObjectBuffer().tell();
        }

        inline bool seek(uint32_t pos)
        {
            return getCurrentObjectBuffer().seek(pos);
        }

        inline ObjectBuffer& getCurrentObjectBuffer()
        {
            return mObjectBufferTable.at(mObjectBufferIndexStack.top());
        }

        inline const ObjectBuffer& getCurrentObjectBuffer() const
        {
            return mObjectBufferTable.at(mObjectBufferIndexStack.top());
        }

        inline bool currentObjectBufferIsValid() const
        {
            return !mObjectBufferIndexStack.empty();
        }

        template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        inline void serializeArithmetic(T* data, uint32_t count = 1)
        {
            serializeBinary(data, sizeof(T) * count);
        }

        virtual void serializeObject(Object* object) = 0;
        virtual void serializeBinary(void* data, uint32_t count) = 0;

        virtual void serializeType(refl::Type* type, void* data, uint32_t count = 1) override;
        virtual void serializeFundamental(refl::Fundamental* fundamental, void* data, uint32_t count = 1) override;
        virtual void serializeStruct(refl::Struct* structure, void* data, uint32_t count = 1) override;
        virtual void serializeSpecial(refl::Special* special, void* data, uint32_t count = 1) override;

        virtual void serializeStdArray(refl::StdArray* stdArray, void* data, uint32_t count = 1) override;
        virtual void serializeStdPair(refl::StdPair* stdPair, void* data, uint32_t count = 1) override;
        virtual void serializeStdTuple(refl::StdTuple* stdTuple, void* data, uint32_t count = 1) override;

    private:
        Asset* mAsset;

        std::vector<std::string> mStringTable;

        std::vector<ImportItem> mImportTable;

        std::vector<ExportItem> mExportTable;

        std::unordered_map<uint32_t, osg::ref_ptr<Object>> mTempObjects;

        std::vector<ObjectBuffer> mObjectBufferTable;

        std::stack<uint32_t> mObjectBufferIndexStack;
    };
}
