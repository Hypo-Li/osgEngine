#pragma once
#include "Asset.h"
#include "Material.h"
#include <osg/Geometry>

namespace xxx
{
    class MeshRenderer;
    class StaticMesh : public Asset
    {
        friend class MeshRenderer;
    public:
        StaticMesh(const fs::path& assetPath) : Asset(assetPath) {}
        virtual ~StaticMesh() = default;

        virtual bool load(const fs::path& assetPath);
        virtual bool save(const fs::path& assetPath) const;

    private:
        std::vector<osg::ref_ptr<osg::Geometry>> _submeshes;
        std::vector<osg::ref_ptr<Material>> _previewMaterial;
    };
}
