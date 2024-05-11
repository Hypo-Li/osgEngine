#pragma once
#include "Asset.h"
#include "MaterialAsset.h"
#include <osg/Geometry>

namespace xxx
{
    class MeshRenderer;
    class MeshAsset : public Asset
    {
        friend class MeshRenderer;
    public:
        MeshAsset(const fs::path& assetPath) : Asset(assetPath) {}
        virtual ~MeshAsset() = default;

        virtual bool load(const fs::path& assetPath);
        virtual bool save(const fs::path& assetPath) const;

    private:
        std::vector<osg::ref_ptr<osg::Geometry>> _submeshes;
        std::vector<osg::ref_ptr<MaterialAsset>> _previewMaterial;
    };
}
