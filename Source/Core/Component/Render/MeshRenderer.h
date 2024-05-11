#pragma once
#include <Core/Runtime/Component.h>
#include <Core/Asset/Mesh.h>

namespace xxx
{
    class MeshRenderer : public Component
    {
    public:
        MeshRenderer() = default;
        virtual ~MeshRenderer() = default;

        void setMesh(Mesh* mesh)
        {

        }


    private:
        osg::ref_ptr<Mesh> _mesh;
    };
}
