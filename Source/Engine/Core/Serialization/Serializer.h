#pragma once
#include <Engine/Core/Object.h>

namespace xxx
{
    class Serializer
    {
    public:
        virtual void serializeType(refl::Type* type, void* data, size_t count = 1);
        virtual void serializeFundamental(refl::Fundamental* fundamental, void* data, size_t count = 1);
        virtual void serializeEnumeration(refl::Enumeration* enumeration, void* data, size_t count = 1);
        virtual void serializeStructure(refl::Structure* structure, void* data, size_t count = 1);
        virtual void serializeClass(refl::Class* clazz, void* data, size_t count = 1);
        virtual void serializeSpecial(refl::Special* special, void* data, size_t count = 1);

        virtual void serializeStdArray(refl::StdArray* stdArray, void* data, size_t count = 1);
        virtual void serializeStdList(refl::StdList* stdList, void* data, size_t count = 1);
        virtual void serializeStdMap(refl::StdMap* stdMap, void* data, size_t count = 1);
        virtual void serializeStdPair(refl::StdPair* stdPair, void* data, size_t count = 1);
        virtual void serializeStdSet(refl::StdSet* stdSet, void* data, size_t count = 1);
        virtual void serializeStdString(refl::StdString* stdString, void* data, size_t count = 1);
        virtual void serializeStdUnorderedMap(refl::StdUnorderedMap* stdUnorderedMap, void* data, size_t count = 1);
        virtual void serializeStdUnorderedSet(refl::StdUnorderedSet* stdUnorderedSet, void* data, size_t count = 1);
        virtual void serializeStdVariant(refl::StdVariant* stdVariant, void* data, size_t count = 1);
        virtual void serializeStdVector(refl::StdVector* stdVector, void* data, size_t count = 1);
    };
}
