#include "Serializer.h"

namespace xxx
{
    const refl::Type* Serializer::sBoolType = refl::Reflection::getType<bool>();
    const refl::Type* Serializer::sCharType = refl::Reflection::getType<char>();
    const refl::Type* Serializer::sWCharType = refl::Reflection::getType<wchar_t>();
    const refl::Type* Serializer::sInt8Type = refl::Reflection::getType<int8_t>();
    const refl::Type* Serializer::sInt16Type = refl::Reflection::getType<int16_t>();
    const refl::Type* Serializer::sInt32Type = refl::Reflection::getType<int32_t>();
    const refl::Type* Serializer::sInt64Type = refl::Reflection::getType<int64_t>();
    const refl::Type* Serializer::sUint8Type = refl::Reflection::getType<uint8_t>();
    const refl::Type* Serializer::sUint16Type = refl::Reflection::getType<uint16_t>();;
    const refl::Type* Serializer::sUint32Type = refl::Reflection::getType<uint64_t>();;
    const refl::Type* Serializer::sUint64Type = refl::Reflection::getType<uint64_t>();;
    const refl::Type* Serializer::sFloatType = refl::Reflection::getType<float>();
    const refl::Type* Serializer::sDoubleType = refl::Reflection::getType<double>();;
}
