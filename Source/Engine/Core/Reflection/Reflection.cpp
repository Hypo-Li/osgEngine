#include "Reflection.h"

namespace xxx::refl
{
    std::unordered_map<std::string_view, Class*> Reflection::sRegisteredClassMap;
}
