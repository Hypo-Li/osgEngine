#pragma once
#include <Engine/Core/Component.h>

namespace xxx
{
    class StaticMeshRenderer : public Component
    {
    public:
        StaticMeshRenderer();
        virtual ~StaticMeshRenderer() = default;

    private:
        osg::ref_ptr<AStaticMesh> _mesh;
        std::vector<std::pair<osg::ref_ptr<AMaterial>, bool>> _materials;
        /**
        *              LOD                  # LOD
        *             /   \
        *       Geode0     Geode1           # Mesh
        *       /               \
        * Geometry00         Geometry01     # Submesh (Material)
        */
        std::vector<osg::ref_ptr<osg::Group>> _groups;
        std::vector<osg::ref_ptr<osg::Geometry>> _geometries;
    };
}
