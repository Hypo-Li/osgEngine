#pragma once
#include <Engine/Core/Component.h>
#include <Engine/Asset/Mesh.h>

namespace xxx
{
    class CStaticMeshRenderer : public Component
    {
    public:
        CStaticMeshRenderer();
        virtual ~CStaticMeshRenderer() = default;

    private:
        osg::ref_ptr<AStaticMesh> _mesh;
        std::vector<std::pair<osg::ref_ptr<AMaterial>, bool>> _materials;
        /**
        *           Group
        *             |
        *            LOD
        *           /   \
        *       Geode0 Geode1
        *      /
        * Geometry0
        */
        std::vector<osg::ref_ptr<osg::Group>> _groups;
        std::vector<osg::ref_ptr<osg::Geometry>> _geometries;
    };
}
