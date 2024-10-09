#pragma once
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

        std::string toString() const
        {
            std::string result(36, '-');
            static constexpr char table[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
            result[0] = table[(A & 0xF0000000) >> 28];
            result[1] = table[(A & 0x0F000000) >> 24];
            result[2] = table[(A & 0x00F00000) >> 20];
            result[3] = table[(A & 0x000F0000) >> 16];
            result[4] = table[(A & 0x0000F000) >> 12];
            result[5] = table[(A & 0x00000F00) >> 8];
            result[6] = table[(A & 0x000000F0) >> 4];
            result[7] = table[(A & 0x0000000F)];

            result[9] = table[(B & 0xF0000000) >> 28];
            result[10] = table[(B & 0x0F000000) >> 24];
            result[11] = table[(B & 0x00F00000) >> 20];
            result[12] = table[(B & 0x000F0000) >> 16];

            result[14] = table[(B & 0x0000F000) >> 12];
            result[15] = table[(B & 0x00000F00) >> 8];
            result[16] = table[(B & 0x000000F0) >> 4];
            result[17] = table[(B & 0x0000000F)];

            result[19] = table[(C & 0xF0000000) >> 28];
            result[20] = table[(C & 0x0F000000) >> 24];
            result[21] = table[(C & 0x00F00000) >> 20];
            result[22] = table[(C & 0x000F0000) >> 16];

            result[24] = table[(C & 0x0000F000) >> 12];
            result[25] = table[(C & 0x00000F00) >> 8];
            result[26] = table[(C & 0x000000F0) >> 4];
            result[27] = table[(C & 0x0000000F)];
            result[28] = table[(D & 0xF0000000) >> 28];
            result[29] = table[(D & 0x0F000000) >> 24];
            result[30] = table[(D & 0x00F00000) >> 20];
            result[31] = table[(D & 0x000F0000) >> 16];
            result[32] = table[(D & 0x0000F000) >> 12];
            result[33] = table[(D & 0x00000F00) >> 8];
            result[34] = table[(D & 0x000000F0) >> 4];
            result[35] = table[(D & 0x0000000F)];
            return result;
        }

        static Guid newGuid();
    };
}

namespace xxx::refl
{
    template <> inline Type* Reflection::createType<Guid>()
    {
        Struct* structGuid = new StructInstance<Guid>("Guid");
        Property* propA = structGuid->addProperty("A", &Guid::A);
        Property* propB = structGuid->addProperty("B", &Guid::B);
        Property* propC = structGuid->addProperty("C", &Guid::C);
        Property* propD = structGuid->addProperty("D", &Guid::D);
        return structGuid;
    }
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
