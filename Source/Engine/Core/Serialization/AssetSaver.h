#pragma once
#include "AssetSerializer.h"

namespace xxx
{
    class AssetSaver : public AssetSerializer
    {
    public:
        AssetSaver();
        virtual ~AssetSaver() = default;

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
