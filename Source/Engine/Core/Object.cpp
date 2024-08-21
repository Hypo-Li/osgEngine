#include "Object.h"

#include <objbase.h>

namespace xxx
{
    Guid Guid::newGuid()
    {
        Guid result;
        if (CoCreateGuid((GUID*)&result) == S_OK)
            return result;
        return result;
    }


}
