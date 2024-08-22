#include "AssetSaver.h"

namespace xxx
{
    using namespace refl;
    void AssetSaver::serialize(Object*& object)
    {
        Class* clazz = object->getClass();
        const auto& properties = clazz->getProperties();
        for (Property* prop : properties)
        {
            bool useAccessor = false;
            void* valuePtr = prop->getValuePtr(&object);
            if (valuePtr == nullptr) // if set/get by setter/getter
            {
                valuePtr = new uint8_t[prop->getType()->getSize()];
                useAccessor = true;
            }

            Type* type = prop->getType();
            switch (type->getKind())
            {
            case refl::Type::Kind::Fundamental:
            {
                if (type == Reflection::BoolType)
                    serialize(*(bool*)(valuePtr));
                else if (type == Reflection::CharType)
                    serialize(*(char*)(valuePtr));
                else if (type == Reflection::WCharType)
                    serialize(*(wchar_t*)(valuePtr));
                else if (type == Reflection::Int8Type)
                    serialize(*(int8_t*)(valuePtr));
                else if (type == Reflection::Int16Type)
                    serialize(*(int16_t*)(valuePtr));
                else if (type == Reflection::Int32Type)
                    serialize(*(int32_t*)(valuePtr));
                else if (type == Reflection::Int64Type)
                    serialize(*(int64_t*)(valuePtr));
                else if (type == Reflection::Uint8Type)
                    serialize(*(uint8_t*)(valuePtr));
                else if (type == Reflection::Uint16Type)
                    serialize(*(uint16_t*)(valuePtr));
                else if (type == Reflection::Uint32Type)
                    serialize(*(uint32_t*)(valuePtr));
                else if (type == Reflection::Uint64Type)
                    serialize(*(uint64_t*)(valuePtr));
                else if (type == Reflection::FloatType)
                    serialize(*(float*)(valuePtr));
                else if (type == Reflection::DoubleType)
                    serialize(*(double*)(valuePtr));
                break;
            }
            case refl::Type::Kind::Enum:
            {
                serialize(*(int64_t*)(valuePtr));
            }
            case refl::Type::Kind::Struct:
            {

            }
            case refl::Type::Kind::Class:
            case refl::Type::Kind::Special:
            default:
                break;
            }
        }
    }
}
