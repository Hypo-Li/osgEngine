#include "Reflection.h"

namespace xxx::refl
{
    const Fundamental* Reflection::BoolType   = dynamic_cast<Fundamental*>(Reflection::getType<bool>());
    const Fundamental* Reflection::CharType   = dynamic_cast<Fundamental*>(Reflection::getType<char>());
    const Fundamental* Reflection::WCharType  = dynamic_cast<Fundamental*>(Reflection::getType<wchar_t>());
    const Fundamental* Reflection::Int8Type   = dynamic_cast<Fundamental*>(Reflection::getType<int8_t>());
    const Fundamental* Reflection::Int16Type  = dynamic_cast<Fundamental*>(Reflection::getType<int16_t>());
    const Fundamental* Reflection::Int32Type  = dynamic_cast<Fundamental*>(Reflection::getType<int32_t>());
    const Fundamental* Reflection::Int64Type  = dynamic_cast<Fundamental*>(Reflection::getType<int64_t>());
    const Fundamental* Reflection::Uint8Type  = dynamic_cast<Fundamental*>(Reflection::getType<uint8_t>());
    const Fundamental* Reflection::Uint16Type = dynamic_cast<Fundamental*>(Reflection::getType<uint16_t>());
    const Fundamental* Reflection::Uint32Type = dynamic_cast<Fundamental*>(Reflection::getType<uint64_t>());
    const Fundamental* Reflection::Uint64Type = dynamic_cast<Fundamental*>(Reflection::getType<uint64_t>());
    const Fundamental* Reflection::FloatType  = dynamic_cast<Fundamental*>(Reflection::getType<float>());
    const Fundamental* Reflection::DoubleType = dynamic_cast<Fundamental*>(Reflection::getType<double>());
}
