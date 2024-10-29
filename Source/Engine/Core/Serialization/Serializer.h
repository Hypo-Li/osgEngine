#pragma once
#include "../Reflection/Reflection.h"

#include <osg/Referenced>
#include <osg/ref_ptr>

#include <string>
#include <vector>
#include <map>
#include <string_view>

namespace xxx
{
    class Object;
    class Serializer : public osg::Referenced
    {
    public:
        virtual ~Serializer() = default;

        virtual void serialize(Object** object) = 0;

        virtual bool isLoader() const = 0;

        virtual bool isSaver() const = 0;

    protected:
        virtual void serializeType(refl::Type* type, void* data, size_t count = 1) = 0;
        virtual void serializeFundamental(refl::Fundamental* fundamental, void* data, size_t count = 1) = 0;
        virtual void serializeStructure(refl::Structure* structure, void* data, size_t count = 1) = 0;
        virtual void serializeSpecial(refl::Special* special, void* data, size_t count = 1) = 0;
        virtual void serializeEnumeration(refl::Enumeration* enumeration, void* data, size_t count = 1) = 0;
        virtual void serializeClass(Object** data, size_t count = 1) = 0;

        virtual void serializeStdString(std::string* data, size_t count = 1) = 0;
        virtual void serializeStdArray(refl::StdArray* stdArray, void* data, size_t count = 1) = 0;
        virtual void serializeStdMap(refl::StdMap* stdMap, void* data, size_t count = 1) = 0;
        virtual void serializeStdPair(refl::StdPair* stdPair, void* data, size_t count = 1) = 0;
        virtual void serializeStdSet(refl::StdSet* stdSet, void* data, size_t count = 1) = 0;
        virtual void serializeStdTuple(refl::StdTuple* stdTuple, void* data, size_t count = 1) = 0;
        virtual void serializeStdUnorderedMap(refl::StdUnorderedMap* stdUnorderedMap, void* data, size_t count = 1) = 0;
        virtual void serializeStdUnorderedSet(refl::StdUnorderedSet* stdUnorderedSet, void* data, size_t count = 1) = 0;
        virtual void serializeStdVariant(refl::StdVariant* stdVariant, void* data, size_t count = 1) = 0;
        virtual void serializeStdVector(refl::StdVector* stdVector, void* data, size_t count = 1) = 0;
    };
}
