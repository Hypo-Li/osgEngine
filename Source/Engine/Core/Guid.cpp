#include "Guid.h"

#include <objbase.h>

namespace xxx
{
    std::string Guid::toString() const
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

    Guid Guid::newGuid()
    {
        Guid result;
        CoCreateGuid((GUID*)&result);
        return result;
    }

    namespace refl
    {
        template <> Type* Reflection::createType<Guid>()
        {
            Struct* structure = new StructInstance<Guid>("Guid");
            structure->addProperty("A", &Guid::A);
            structure->addProperty("B", &Guid::B);
            structure->addProperty("C", &Guid::C);
            structure->addProperty("D", &Guid::D);
            return structure;
        }
    }
}
