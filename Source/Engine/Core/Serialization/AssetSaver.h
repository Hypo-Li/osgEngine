#pragma once
#include "AssetSerializer.h"

namespace xxx
{
    class AssetSaver : public AssetSerializer
    {
    public:
        AssetSaver(Asset* asset) : AssetSerializer(asset)
        {

        }
        virtual ~AssetSaver() = default;

    protected:
        virtual void serializeObject(Object* object) override;
        virtual void serializeBinary(void* data, size_t count) override;

        virtual void serializeEnum(refl::Enum* enumerate, void* data, size_t count = 1) override;
        virtual void serializeClass(Object** data, size_t count = 1) override;

        virtual void serializeStdString(std::string* data, size_t count = 1) override;
        virtual void serializeStdMap(refl::StdMap* stdMap, void* data, size_t count = 1) override;
        virtual void serializeStdSet(refl::StdSet* stdSet, void* data, size_t count = 1) override;
        virtual void serializeStdUnorderedMap(refl::StdUnorderedMap* stdUnorderedMap, void* data, size_t count = 1) override;
        virtual void serializeStdUnorderedSet(refl::StdUnorderedSet* stdUnorderedSet, void* data, size_t count = 1) override;
        virtual void serializeStdVariant(refl::StdVariant* stdVariant, void* data, size_t count = 1) override;
        virtual void serializeStdVector(refl::StdVector* stdVector, void* data, size_t count = 1) override;
    };
}
