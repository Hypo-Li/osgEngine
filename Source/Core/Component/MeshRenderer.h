#pragma once
#include <Core/Base/Component.h>
//#include <Core/Asset/StaticMesh.h>

namespace xxx
{
    class MeshRenderer : public Component
    {
    public:
        MeshRenderer() = default;
        virtual ~MeshRenderer() = default;

        /*void setMesh(StaticMesh* mesh)
        {

        }

        void setMaterial(Material* material, uint32_t index)
        {

        }*/

        void setMesh(osg::Node* node)
        {
            Group::addChild(node);
        }

    private:
        /*osg::ref_ptr<StaticMesh> _mesh;
        std::vector<osg::ref_ptr<osg::Geode>> _materialNode;
        std::vector<osg::ref_ptr<Material>> _materials;*/
    };
}
