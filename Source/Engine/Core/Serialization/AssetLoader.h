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

        virtual bool isLoading() const override
        {
            return false;
        }

        virtual bool isSaving() const override
        {
            return true;
        }

        virtual void serializeObject(Object*& object) override;

        void serializeClass(refl::Class* clazz, void* data, size_t count = 1);
    };
}
