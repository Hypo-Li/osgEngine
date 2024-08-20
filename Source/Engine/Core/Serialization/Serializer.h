#pragma once
#include "../Object.h"

#include <string>
#include <vector>
#include <map>
#include <string_view>

namespace xxx
{
    class Serializer
    {
    public:
        virtual ~Serializer() = default;

        virtual void serialize(bool& value) = 0;
        virtual void serialize(char& value) = 0;
        virtual void serialize(int8_t& value) = 0;
        virtual void serialize(int16_t& value) = 0;
        virtual void serialize(int32_t& value) = 0;
        virtual void serialize(int64_t& value) = 0;
        virtual void serialize(uint8_t& value) = 0;
        virtual void serialize(uint16_t& value) = 0;
        virtual void serialize(uint32_t& value) = 0;
        virtual void serialize(uint64_t& value) = 0;
        virtual void serialize(float& value) = 0;
        virtual void serialize(double& value) = 0;
        virtual void serialize(std::string& value) = 0;

        //virtual void serialize(Object*& value)
        //{
        //    refl::Class* clazz = value->getClass();
        //    const auto& properties = clazz->getProperties();
        //    for (refl::Property* prop : properties)
        //    {
        //        void* valuePtr = prop->getValuePtr(&value);
        //        if (valuePtr == nullptr) // set/get by setter/getter
        //        {
        //            valuePtr = new uint8_t[prop->getType()->getSize()];
        //            prop->getValue(&value, valuePtr);
        //        }
        //        refl::Type* type = prop->getType();
        //        switch (type->getKind())
        //        {
        //        case refl::Type::Kind::Fundamental:
        //        {
        //            if (type == refl::Reflection::getType<bool>())
        //                serialize(*(bool*)(valuePtr));
        //            else if (type == refl::Reflection::getType<char>())
        //                serialize(*(char*)(valuePtr));
        //        }
        //        case refl::Type::Kind::Enum:
        //        case refl::Type::Kind::Struct:
        //        case refl::Type::Kind::Class:
        //        case refl::Type::Kind::Special:
        //        default:
        //            break;
        //        }
        //    }
        //}
    };
}
