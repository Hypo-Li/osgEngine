#pragma once
#include "Serializer.h"
#include "../Object.h"

namespace xxx
{
    class AssetSaver : public Serializer
    {
    public:
        virtual ~AssetSaver() = default;

        virtual bool isLoading() const override
        {
            return false;
        }

        virtual bool isSaving() const override
        {
            return true;
        }

        virtual void serialize(bool& value);
        virtual void serialize(char& value);
        virtual void serialize(wchar_t& value);
        virtual void serialize(int8_t& value);
        virtual void serialize(int16_t& value);
        virtual void serialize(int32_t& value);
        virtual void serialize(int64_t& value);
        virtual void serialize(uint8_t& value);
        virtual void serialize(uint16_t& value);
        virtual void serialize(uint32_t& value);
        virtual void serialize(uint64_t& value);
        virtual void serialize(float& value);
        virtual void serialize(double& value);
        virtual void serialize(std::string& value);

        virtual void serialize(Object*& object) override
        {
            refl::Class* clazz = object->getClass();
            const auto& properties = clazz->getProperties();
            for (refl::Property* prop : properties)
            {
                bool useAccessor = false;
                void* valuePtr = prop->getValuePtr(&object);
                if (valuePtr == nullptr) // if set/get by setter/getter
                {
                    valuePtr = new uint8_t[prop->getType()->getSize()];
                    useAccessor = true;
                }

                refl::Type* type = prop->getType();
                switch (type->getKind())
                {
                case refl::Type::Kind::Fundamental:
                {
                    if (type == sBoolType)
                        serialize(*(bool*)(valuePtr));
                    else if (type == sCharType)
                        serialize(*(char*)(valuePtr));
                    else if (type == sWCharType)
                        serialize(*(wchar_t*)(valuePtr));
                    else if (type == sInt8Type)
                        serialize(*(int8_t*)(valuePtr));
                    else if (type == sInt16Type)
                        serialize(*(int16_t*)(valuePtr));
                    else if (type == sInt32Type)
                        serialize(*(int32_t*)(valuePtr));
                    else if (type == sInt64Type)
                        serialize(*(int64_t*)(valuePtr));
                    else if (type == sUint8Type)
                        serialize(*(uint8_t*)(valuePtr));
                    else if (type == sUint16Type)
                        serialize(*(uint16_t*)(valuePtr));
                    else if (type == sUint32Type)
                        serialize(*(uint32_t*)(valuePtr));
                    else if (type == sUint64Type)
                        serialize(*(uint64_t*)(valuePtr));
                    else if (type == sFloatType)
                        serialize(*(float*)(valuePtr));
                    else if (type == sDoubleType)
                        serialize(*(double*)(valuePtr));
                    break;
                }
                case refl::Type::Kind::Enum:
                {

                }
                case refl::Type::Kind::Struct:
                case refl::Type::Kind::Class:
                case refl::Type::Kind::Special:
                default:
                    break;
                }
            }
        }

    private:

    };
}
