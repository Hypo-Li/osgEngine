#include "Serializer.h"

namespace xxx
{
    using namespace refl;

    void Serializer::serializeType(refl::Type* type, void* data, size_t count)
    {
        switch (type->getKind())
        {
        case refl::Type::Kind::Fundamental:
        {
            serializeFundamental(dynamic_cast<Fundamental*>(type), data, count);
            break;
        }
        case refl::Type::Kind::Enumeration:
        {
            serializeEnumeration(dynamic_cast<Enumeration*>(type), data, count);
            break;
        }
        case refl::Type::Kind::Structure:
        {
            serializeStructure(dynamic_cast<Structure*>(type), data, count);
            break;
        }
        case refl::Type::Kind::Class:
        {
            serializeClass(static_cast<Object**>(data), count);
            break;
        }
        case refl::Type::Kind::Special:
        {
            serializeSpecial(dynamic_cast<Special*>(type), data, count);
            break;
        }
        default:
            break;
        }
    }

    void Serializer::serializeFundamental(refl::Fundamental* fundamental, void* data, size_t count) {}

    void Serializer::serializeEnumeration(refl::Enumeration* enumeration, void* data, size_t count) {}

    void Serializer::serializeStructure(refl::Structure* structure, void* data, size_t count) {}

    void Serializer::serializeClass(Object** data, size_t count) {}

    void Serializer::serializeSpecial(refl::Special* special, void* data, size_t count)
    {
        switch (special->getSpecialType())
        {
        case SpecialType::Std_String:
        {
            serializeStdString(static_cast<std::string*>(data), count);
            break;
        }
        case SpecialType::Std_Array:
        {
            serializeStdArray(dynamic_cast<StdArray*>(special), data, count);
            break;
        }
        case SpecialType::Std_Map:
        {
            serializeStdMap(dynamic_cast<StdMap*>(special), data, count);
            break;
        }
        case SpecialType::Std_Pair:
        {
            serializeStdPair(dynamic_cast<StdPair*>(special), data, count);
            break;
        }
        case SpecialType::Std_Set:
        {
            serializeStdSet(dynamic_cast<StdSet*>(special), data, count);
            break;
        }
        case SpecialType::Std_Tuple:
        {
            serializeStdTuple(dynamic_cast<StdTuple*>(special), data, count);
            break;
        }
        case SpecialType::Std_Unordered_Map:
        {
            serializeStdUnorderedMap(dynamic_cast<StdUnorderedMap*>(special), data, count);
            break;
        }
        case SpecialType::Std_Unordered_Set:
        {
            serializeStdUnorderedSet(dynamic_cast<StdUnorderedSet*>(special), data, count);
            break;
        }
        case SpecialType::Std_Variant:
        {
            serializeStdVariant(dynamic_cast<StdVariant*>(special), data, count);
            break;
        }
        case SpecialType::Std_Vector:
        {
            serializeStdVector(dynamic_cast<StdVector*>(special), data, count);
            break;
        }
        default:
            break;
        }
    }

    void Serializer::serializeStdString(std::string* data, size_t count) {}

    void Serializer::serializeStdArray(refl::StdArray* stdArray, void* data, size_t count) {}

    void Serializer::serializeStdMap(refl::StdMap* stdMap, void* data, size_t count) {}

    void Serializer::serializeStdPair(refl::StdPair* stdPair, void* data, size_t count) {}

    void Serializer::serializeStdSet(refl::StdSet* stdSet, void* data, size_t count) {}

    void Serializer::serializeStdTuple(refl::StdTuple* stdTuple, void* data, size_t count) {}

    void Serializer::serializeStdUnorderedMap(refl::StdUnorderedMap* stdUnorderedMap, void* data, size_t count) {}

    void Serializer::serializeStdUnorderedSet(refl::StdUnorderedSet* stdUnorderedSet, void* data, size_t count) {}

    void Serializer::serializeStdVariant(refl::StdVariant* stdVariant, void* data, size_t count) {}

    void Serializer::serializeStdVector(refl::StdVector* stdVector, void* data, size_t count) {}
}
