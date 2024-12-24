#include "Serializer.h"

namespace xxx
{
    using namespace refl;

    void Serializer::serializeType(Type* type, void* data, size_t count)
    {
        switch (type->getKind())
        {
        case Type::Kind::Fundamental:
            serializeFundamental(dynamic_cast<Fundamental*>(type), data, count);
            break;
        case Type::Kind::Enumeration:
            serializeEnumeration(dynamic_cast<Enumeration*>(type), data, count);
            break;
        case Type::Kind::Structure:
            serializeStructure(dynamic_cast<Structure*>(type), data, count);
            break;
        case Type::Kind::Class:
            serializeClass(dynamic_cast<Class*>(type), data, count);
            break;
        case Type::Kind::Special:
            serializeSpecial(dynamic_cast<Special*>(type), data, count);
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
            serializeStdArray(dynamic_cast<StdArray*>(special), data, count);
            break;
        case Special::Case::StdList:
            serializeStdList(dynamic_cast<StdList*>(special), data, count);
            break;
        case Special::Case::StdMap:
            serializeStdMap(dynamic_cast<StdMap*>(special), data, count);
            break;
        case Special::Case::StdPair:
            serializeStdPair(dynamic_cast<StdPair*>(special), data, count);
            break;
        case Special::Case::StdSet:
            serializeStdSet(dynamic_cast<StdSet*>(special), data, count);
            break;
        case Special::Case::StdString:
            serializeStdString(dynamic_cast<StdString*>(special), data, count);
            break;
        case Special::Case::StdUnorderedMap:
            serializeStdUnorderedMap(dynamic_cast<StdUnorderedMap*>(special), data, count);
            break;
        case Special::Case::StdUnorderedSet:
            serializeStdUnorderedSet(dynamic_cast<StdUnorderedSet*>(special), data, count);
            break;
        case Special::Case::StdVariant:
            serializeStdVariant(dynamic_cast<StdVariant*>(special), data, count);
            break;
        case Special::Case::StdVector:
            serializeStdVector(dynamic_cast<StdVector*>(special), data, count);
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
