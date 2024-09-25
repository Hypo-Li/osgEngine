#pragma once
#include "../Object.h"

#include <string>
#include <vector>
#include <map>
#include <string_view>

namespace xxx
{
    class Serializer : public osg::Referenced
    {
    public:
        virtual ~Serializer() = default;

        virtual void serialize(Object** object) = 0;

    protected:
        virtual void serializeType(refl::Type* type, void* data, uint32_t count = 1) = 0;
        virtual void serializeFundamental(refl::Fundamental* fundamental, void* data, uint32_t count = 1) = 0;
        virtual void serializeStruct(refl::Struct* structure, void* data, uint32_t count = 1) = 0;
        virtual void serializeSpecial(refl::Special* special, void* data, uint32_t count = 1) = 0;
        virtual void serializeEnum(refl::Enum* enumerate, void* data, uint32_t count = 1) = 0;
        virtual void serializeClass(Object** data, uint32_t count = 1) = 0;

        virtual void serializeStdArray(refl::StdArray* stdArray, void* data, uint32_t count = 1) = 0;
        virtual void serializeStdPair(refl::StdPair* stdPair, void* data, uint32_t count = 1) = 0;
        virtual void serializeStdTuple(refl::StdTuple* stdTuple, void* data, uint32_t count = 1) = 0;
        virtual void serializeStdString(std::string* data, uint32_t count = 1) = 0;
        virtual void serializeStdSet(refl::StdSet* stdSet, void* data, uint32_t count = 1) = 0;
        virtual void serializeStdMap(refl::StdMap* stdMap, void* data, uint32_t count = 1) = 0;
        virtual void serializeStdVariant(refl::StdVariant* stdVariant, void* data, uint32_t count = 1) = 0;
        virtual void serializeStdVector(refl::StdVector* stdVector, void* data, uint32_t count = 1) = 0;
    };
}
