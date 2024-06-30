#pragma once

namespace xxx
{
#ifdef REFLECTION_PARSER
#else
#define ENUM(ENUM_NAME, ...) enum ENUM_NAME
#define CLASS(CLASS_NAME, ...) class CLASS_NAME
#define STRUCT(STRUCT_NAME, ...) struct STRUCT_NAME
#define PROPERTY(...)
#define FUNCTION(...)
#endif // REFLECTION_PARSER
}
