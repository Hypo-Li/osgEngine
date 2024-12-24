#pragma once
#include "AssetSerializer.h"

namespace xxx
{
    class AssetLoader : public AssetSerializer
    {
    public:
        AssetLoader(Asset* asset);

        virtual void serializeEnumeration(refl::Enumeration* enumeration, void* data, size_t count = 1) override;
        virtual void serializeClass(refl::Class* clazz, void* data, size_t count = 1) override;

        virtual void serializeStdArray(refl::StdArray* stdArray, void* data, size_t count = 1) override;
        virtual void serializeStdList(refl::StdList* stdList, void* data, size_t count = 1) override;
        virtual void serializeStdMap(refl::StdMap* stdMap, void* data, size_t count = 1) override;
        virtual void serializeStdPair(refl::StdPair* stdPair, void* data, size_t count = 1) override;
        virtual void serializeStdSet(refl::StdSet* stdSet, void* data, size_t count = 1) override;
        virtual void serializeStdString(refl::StdString* stdString, void* data, size_t count = 1) override;
        virtual void serializeStdUnorderedMap(refl::StdUnorderedMap* stdUnorderedMap, void* data, size_t count = 1) override;
        virtual void serializeStdUnorderedSet(refl::StdUnorderedSet* stdUnorderedSet, void* data, size_t count = 1) override;
        virtual void serializeStdVariant(refl::StdVariant* stdVariant, void* data, size_t count = 1) override;
        virtual void serializeStdVector(refl::StdVector* stdVector, void* data, size_t count = 1) override;

    protected:
        virtual void serializeBinary(void* data, size_t size) override;

    private:
        std::unordered_map<uint32_t, Object*> mObjectsTemp;

        void serializeObject(Object* object);
    };
}
