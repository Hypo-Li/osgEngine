#include "Reflection.h"

namespace xxx::refl
{
    const refl::Type* Reflection::BoolType = refl::Reflection::getType<bool>();
    const refl::Type* Reflection::CharType = refl::Reflection::getType<char>();
    const refl::Type* Reflection::WCharType = refl::Reflection::getType<wchar_t>();
    const refl::Type* Reflection::Int8Type = refl::Reflection::getType<int8_t>();
    const refl::Type* Reflection::Int16Type = refl::Reflection::getType<int16_t>();
    const refl::Type* Reflection::Int32Type = refl::Reflection::getType<int32_t>();
    const refl::Type* Reflection::Int64Type = refl::Reflection::getType<int64_t>();
    const refl::Type* Reflection::Uint8Type = refl::Reflection::getType<uint8_t>();
    const refl::Type* Reflection::Uint16Type = refl::Reflection::getType<uint16_t>();
    const refl::Type* Reflection::Uint32Type = refl::Reflection::getType<uint64_t>();
    const refl::Type* Reflection::Uint64Type = refl::Reflection::getType<uint64_t>();
    const refl::Type* Reflection::FloatType = refl::Reflection::getType<float>();
    const refl::Type* Reflection::DoubleType = refl::Reflection::getType<double>();

    std::unordered_map<std::string_view, Class*> Reflection::sRegisteredClassMap;
}
