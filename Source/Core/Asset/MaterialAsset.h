#pragma once
#include "Asset.h"
#include <osg/StateSet>

namespace xxx
{
    class MaterialAsset : public Asset
    {
    public:
        MaterialAsset(const fs::path& assetPath) : Asset(assetPath) {}
        virtual ~MaterialAsset() = default;

        virtual bool load(const fs::path& assetPath);
        virtual bool save(const fs::path& assetPath) const;

    private:
        osg::ref_ptr<osg::StateSet> _material;
        
    };
}
