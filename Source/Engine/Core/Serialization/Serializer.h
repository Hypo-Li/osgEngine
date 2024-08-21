#pragma once
#include "../Object.h"

#include <string>
#include <vector>
#include <map>
#include <string_view>

namespace xxx
{
    class Serializer
    {
    public:
        virtual ~Serializer() = default;

        virtual bool isLoading() const = 0;
        virtual bool isSaving() const = 0;

        virtual void serialize(bool& value) = 0;
        virtual void serialize(char& value) = 0;
        virtual void serialize(wchar_t& value) = 0;
        virtual void serialize(int8_t& value) = 0;
        virtual void serialize(int16_t& value) = 0;
        virtual void serialize(int32_t& value) = 0;
        virtual void serialize(int64_t& value) = 0;
        virtual void serialize(uint8_t& value) = 0;
        virtual void serialize(uint16_t& value) = 0;
        virtual void serialize(uint32_t& value) = 0;
        virtual void serialize(uint64_t& value) = 0;
        virtual void serialize(float& value) = 0;
        virtual void serialize(double& value) = 0;
        virtual void serialize(std::string& value) = 0;

        virtual void serialize(Object*& object) = 0;

    protected:
        static const refl::Type* sBoolType;
        static const refl::Type* sCharType;
        static const refl::Type* sWCharType;
        static const refl::Type* sInt8Type;
        static const refl::Type* sInt16Type;
        static const refl::Type* sInt32Type;
        static const refl::Type* sInt64Type;
        static const refl::Type* sUint8Type;
        static const refl::Type* sUint16Type;
        static const refl::Type* sUint32Type;
        static const refl::Type* sUint64Type;
        static const refl::Type* sFloatType;
        static const refl::Type* sDoubleType;
    };
}
