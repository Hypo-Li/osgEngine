#pragma once
#include "AssetSerializer.h"

namespace xxx
{
    class AssetLoader : public AssetSerializer
    {
    public:
        AssetLoader(Asset* asset) : AssetSerializer(asset)
        {

        }
        virtual ~AssetLoader() = default;

    protected:
        virtual void serializeObject(Object* object) override;
        virtual void serializeBinary(void* data, uint32_t count) override;

        virtual void serializeEnum(refl::Enum* enumerate, void* data, uint32_t count = 1) override;
        virtual void serializeClass(refl::Class* clazz, void* data, uint32_t count = 1) override;

        virtual void serializeStdString(std::string* data, uint32_t count = 1) override;
        virtual void serializeStdMap(refl::StdMap* stdMap, void* data, uint32_t count = 1) override;
        virtual void serializeStdSet(refl::StdSet* stdSet, void* data, uint32_t count = 1) override;
        virtual void serializeStdVariant(refl::StdVariant* stdVariant, void* data, uint32_t count = 1) override;
        virtual void serializeStdVector(refl::StdVector* stdVector, void* data, uint32_t count = 1) override;

    private:
        std::vector<osg::ref_ptr<Object>> mObjectsTemp;
    };
}
