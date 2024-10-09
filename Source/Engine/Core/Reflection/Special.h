#pragma once
#include "Reflection.h"

namespace xxx::refl
{
    enum class SpecialType
    {
        Std_String,
        Std_Array,
        Std_Map,
        Std_Pair,
        Std_Set,
        Std_Tuple,
        Std_Unordered_Map,
        Std_Unordered_Set,
        Std_Variant,
        Std_Vector,
    };

    class Special : public Type
    {
    public:
        virtual Kind getKind() const override { return Kind::Special; }
        virtual SpecialType getSpecialType() const = 0;

    protected:
        virtual ~Special() = default;
    };
}
