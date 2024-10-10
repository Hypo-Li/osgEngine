#pragma once
#include "Reflection/Reflection.h"

#include <string>

namespace xxx
{
    struct Guid
    {
        union
        {
            struct
            {
                uint32_t A;
                uint32_t B;
                uint32_t C;
                uint32_t D;
            };
            struct
            {
                uint64_t data[2];
            };
        };

        Guid()
        {
            data[0] = data[1] = 0;
        }

        bool operator==(const Guid& rhs) const
        {
            return data[0] == rhs.data[0] && data[1] == rhs.data[1];
        }

        bool operator!=(const Guid& rhs) const
        {
            return data[0] != rhs.data[0] || data[1] != rhs.data[1];
        }

        bool isValid() const
        {
            return data[0] != 0 || data[1] != 0;
        }

        std::string toString() const;

        static Guid newGuid();
    };
}

namespace xxx::refl
{
    template <> Type* Reflection::createType<Guid>();
}

namespace std
{
    template <> struct hash<xxx::Guid>
    {
        std::size_t operator()(const xxx::Guid& guid) const
        {
            return std::hash<uint64_t>()(guid.data[0]) ^ std::hash<uint64_t>()(guid.data[1]);
        }
    };
}
