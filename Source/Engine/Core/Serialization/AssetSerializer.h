#pragma once
#include "Serializer.h"
#include "../Asset.h"

namespace xxx
{
    class AssetSerializer : public Serializer
    {
    public:
        virtual bool isLoading() const = 0;
        virtual bool isSaving() const = 0;

    private:
        Asset* mAsset;
        std::vector<uint8_t> mBuffer;
    };
}
