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
            
        }
        virtual ~AssetSerializer() = default;

        template <typename T, typename = std::enable_if_t<std::is_base_of_v<Object, T>>>
        void serialize(T*& object)
        {
            serializeClass(object->getClass(), &object);
        }

    protected:
        inline void serialize(bool* data, uint32_t count = 1)
        {
            serializeBinary(data, sizeof(bool) * count);
        }

        inline void serialize(char* data, uint32_t count = 1)
        {
            serializeBinary(data, sizeof(char) * count);
        }

        inline void serialize(wchar_t* data, uint32_t count = 1)
        {
            serializeBinary(data, sizeof(wchar_t) * count);
        }

        inline void serialize(int8_t* data, uint32_t count = 1)
        {
            serializeBinary(data, sizeof(int8_t) * count);
        }

        inline void serialize(int16_t* data, uint32_t count = 1)
        {
            serializeBinary(data, sizeof(int16_t) * count);
        }

        inline void serialize(int32_t* data, uint32_t count = 1)
        {
            serializeBinary(data, sizeof(int32_t) * count);
        }

        inline void serialize(int64_t* data, uint32_t count = 1)
        {
            serializeBinary(data, sizeof(int64_t) * count);
        }

        inline void serialize(uint8_t* data, uint32_t count = 1)
        {
            serializeBinary(data, sizeof(uint8_t) * count);
        }

        inline void serialize(uint16_t* data, uint32_t count = 1)
        {
            serializeBinary(data, sizeof(uint16_t) * count);
        }

        inline void serialize(uint32_t* data, uint32_t count = 1)
        {
            serializeBinary(data, sizeof(uint32_t) * count);
        }

        inline void serialize(uint64_t* data, uint32_t count = 1)
        {
            serializeBinary(data, sizeof(uint64_t) * count);
        }

        inline void serialize(float* data, uint32_t count = 1)
        {
            serializeBinary(data, sizeof(float) * count);
        }

        inline void serialize(double* data, uint32_t count = 1)
        {
            serializeBinary(data, sizeof(double) * count);
        }

        virtual void serializeObject(Object*& object) = 0;
        virtual void serializeBinary(void* data, uint32_t count) = 0;

        // serialize types
        virtual void serializeType(refl::Type* type, void* data, uint32_t count = 1);
        virtual void serializeFundamental(refl::Fundamental* fundamental, void* data, uint32_t count = 1);
        virtual void serializeStruct(refl::Struct* structure, void* data, uint32_t count = 1);
        virtual void serializeSpecial(refl::Special* special, void* data, uint32_t count = 1);

        virtual void serializeEnum(refl::Enum* enumerate, void* data, uint32_t count = 1) = 0;
        virtual void serializeClass(refl::Class* clazz, void* data, uint32_t count = 1) = 0;

        // serialize specials
        virtual void serializeStdArray(refl::StdArray* stdArray, void* data, uint32_t count = 1);
        virtual void serializeStdPair(refl::StdPair* stdPair, void* data, uint32_t count = 1);
        virtual void serializeStdTuple(refl::StdTuple* stdTuple, void* data, uint32_t count = 1);

        virtual void serializeStdString(std::string* data, uint32_t count = 1) = 0;
        virtual void serializeStdSet(refl::StdSet* stdSet, void* data, uint32_t count = 1) = 0;
        virtual void serializeStdMap(refl::StdMap* stdMap, void* data, uint32_t count = 1) = 0;
        virtual void serializeStdVariant(refl::StdVariant* stdVariant, void* data, uint32_t count = 1) = 0;
        virtual void serializeStdVector(refl::StdVector* stdVector, void* data, uint32_t count = 1) = 0;

        struct ObjectBuffer
        {
            std::vector<uint8_t> buffer;
            uint32_t pointer = 0;

            ObjectBuffer(uint32_t bufferSize = 0, uint32_t pointer = 0) : buffer(bufferSize), pointer(pointer)
            {

            }
        };

        inline uint32_t createNewObjectBuffer()
        {
            uint32_t index = mObjectBufferTable.size();
            mObjectBufferTable.emplace_back();
            return index;
        }

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
            return getCurrentObjectBuffer().pointer;
        }

        inline void seek(uint32_t pos)
        {
            getCurrentObjectBuffer().pointer = pos;
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

    protected:
        Asset* mAsset;
        std::vector<std::string> mStringTable;
        std::vector<uint32_t> mImportTable;
        std::vector<Guid> mExportTable;

    private:
        std::vector<ObjectBuffer> mObjectBufferTable;
        std::stack<uint32_t> mObjectBufferIndexStack;
    };
}
