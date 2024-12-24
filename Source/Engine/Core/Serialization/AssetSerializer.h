#pragma once
#include "Serializer.h"

#include <Engine/Utility/Guid.h>
#include <stack>

namespace xxx
{
    class Asset;

    class AssetSerializer : public Serializer
    {
    public:
        AssetSerializer(Asset* asset) : mAsset(asset) {}
        virtual ~AssetSerializer() = default;

        virtual void serializeFundamental(refl::Fundamental* fundamental, void* data, size_t count = 1) override;

        virtual void serializeStructure(refl::Structure* structure, void* data, size_t count = 1) override;

        inline Asset* getAsset() const
        {
            return mAsset;
        }

        uint32_t addString(const std::string& str);

        inline const std::string& getString(uint32_t strIndex) const
        {
            return mStringTable[strIndex];
        }

        inline uint32_t getStringTableSize() const
        {
            return mStringTable.size();
        }

        uint32_t addImportItem(uint32_t importItem);

        inline uint32_t getImportItem(uint32_t importIndex) const
        {
            return mImportTable.at(importIndex);
        }

        inline uint32_t getImportTableSize() const
        {
            return mImportTable.size();
        }

        class ObjectBuffer : public osg::Referenced
        {
        public:
            ObjectBuffer(size_t bufferSize = 0) :
                mBuffer(bufferSize), mPointer(0) {}

            inline size_t tell() const
            {
                return mPointer;
            }

            inline bool seek(size_t pos)
            {
                if (pos <= mBuffer.size())
                {
                    mPointer = pos;
                    return true;
                }
                return false;
            }

            inline void write(const void* data, size_t size)
            {
                uint32_t newPointer = mPointer + size;
                if (newPointer > mBuffer.size())
                    mBuffer.resize(newPointer);
                std::memcpy(mBuffer.data() + mPointer, data, size);
                mPointer = newPointer;
            }

            inline void read(void* data, size_t size)
            {
                std::memcpy(data, mBuffer.data() + mPointer, size);
                mPointer += size;
            }

            uint8_t* getData()
            {
                return mBuffer.data();
            }

            size_t getSize() const
            {
                return mBuffer.size();
            }

        private:
            std::vector<uint8_t> mBuffer;
            size_t mPointer;
        };

        inline ObjectBuffer* getCurrentObjectBuffer()
        {
            if (mObjectBufferIndexStack.empty())
                return nullptr;
            uint32_t objectBufferIndex = mObjectBufferIndexStack.top();
            return mObjectBuffers.at(objectBufferIndex);
        }

        inline void pushObjectBuffer(uint32_t objectBufferIndex)
        {
            mObjectBufferIndexStack.push(objectBufferIndex);
        }

        inline void popObjectBuffer()
        {
            if (!mObjectBufferIndexStack.empty())
                mObjectBufferIndexStack.pop();
        }

        inline uint32_t createObjectBuffer(size_t bufferSize = 0)
        {
            mObjectBuffers.emplace_back(new ObjectBuffer(bufferSize));
            return mObjectBuffers.size() - 1;
        }

        inline uint32_t getObjectBufferCount() const
        {
            return mObjectBuffers.size();
        }

    protected:
        virtual void serializeBinary(void* data, size_t size) = 0;

        template <typename T> requires std::is_arithmetic_v<T>
        void serializeArithmetic(T* data, size_t count = 1)
        {
            serializeBinary(data, sizeof(T) * count);
        }

    private:
        Asset* mAsset;
        std::vector<std::string> mStringTable;
        std::unordered_map<std::string, uint32_t> mStringIndexMap;
        std::vector<uint32_t> mImportTable;
        std::stack<uint32_t> mObjectBufferIndexStack;
        std::vector<osg::ref_ptr<ObjectBuffer>> mObjectBuffers;
    };
}
