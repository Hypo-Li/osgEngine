#include "Serializer.h"

namespace xxx
{
    using namespace refl;

    void Serializer::serializeType(Type* type, void* data, size_t count)
    {
        switch (type->getKind())
        {
        case Type::Kind::Fundamental:
            serializeFundamental(static_cast<Fundamental*>(type), data, count);
            break;
        case Type::Kind::Enumeration:
            serializeEnumeration(static_cast<Enumeration*>(type), data, count);
            break;
        case Type::Kind::Structure:
            serializeStructure(static_cast<Structure*>(type), data, count);
            break;
        case Type::Kind::Class:
            serializeClass(static_cast<Class*>(type), data, count);
            break;
        case Type::Kind::Special:
            serializeSpecial(static_cast<Special*>(type), data, count);
            break;
        default:
            break;
        }
    }

    void Serializer::serializeFundamental(Fundamental* fundamental, void* data, size_t count) {}

    void Serializer::serializeEnumeration(Enumeration* enumeration, void* data, size_t count) {}

    void Serializer::serializeStructure(Structure* structure, void* data, size_t count) {}

    void Serializer::serializeClass(Class* clazz, void* data, size_t count) {}

    void Serializer::serializeSpecial(Special* special, void* data, size_t count)
    {
        switch (special->getCase())
        {
        case Special::Case::StdArray:
            serializeStdArray(static_cast<StdArray*>(special), data, count);
            break;
        case Special::Case::StdList:
            serializeStdList(static_cast<StdList*>(special), data, count);
            break;
        case Special::Case::StdMap:
            serializeStdMap(static_cast<StdMap*>(special), data, count);
            break;
        case Special::Case::StdPair:
            serializeStdPair(static_cast<StdPair*>(special), data, count);
            break;
        case Special::Case::StdSet:
            serializeStdSet(static_cast<StdSet*>(special), data, count);
            break;
        case Special::Case::StdString:
            serializeStdString(static_cast<StdString*>(special), data, count);
            break;
        case Special::Case::StdUnorderedMap:
            serializeStdUnorderedMap(static_cast<StdUnorderedMap*>(special), data, count);
            break;
        case Special::Case::StdUnorderedSet:
            serializeStdUnorderedSet(static_cast<StdUnorderedSet*>(special), data, count);
            break;
        case Special::Case::StdVariant:
            serializeStdVariant(static_cast<StdVariant*>(special), data, count);
            break;
        case Special::Case::StdVector:
            serializeStdVector(static_cast<StdVector*>(special), data, count);
            break;
        default:
            break;
        }
    }

    void Serializer::serializeStdArray(StdArray* stdArray, void* data, size_t count) {}

    void Serializer::serializeStdList(StdList* stdList, void* data, size_t count) {}

    void Serializer::serializeStdMap(StdMap* stdMap, void* data, size_t count) {}

    void Serializer::serializeStdPair(StdPair* stdPair, void* data, size_t count) {}

    void Serializer::serializeStdSet(StdSet* stdSet, void* data, size_t count) {}

    void Serializer::serializeStdString(StdString* stdString, void* data, size_t count) {}

    void Serializer::serializeStdUnorderedMap(refl::StdUnorderedMap* stdUnorderedMap, void* data, size_t count) {}

    void Serializer::serializeStdUnorderedSet(refl::StdUnorderedSet* stdUnorderedSet, void* data, size_t count) {}

    void Serializer::serializeStdVariant(StdVariant* stdVariant, void* data, size_t count) {}

    void Serializer::serializeStdVector(StdVector* stdVector, void* data, size_t count) {}
}
