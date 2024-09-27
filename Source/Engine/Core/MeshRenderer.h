#pragma once
#include "Component.h"
#include <Engine/Render/Mesh.h>

#include <osg/LOD>

namespace xxx
{
    class MeshRenderer : public Component
    {
        REFLECT_CLASS(MeshRenderer)
    public:
        MeshRenderer() :
            mOsgGroup(new osg::Group)
        {
            mOsgComponentGroup->addChild(mOsgGroup);
        }

        virtual Type getType() const override
        {
            return Type::MeshRenderer;
        }

        void setMesh(Mesh* mesh)
        {
            mMesh = mesh;
            for (uint32_t i = 0; i < mMesh->mOsgGeometries.size(); ++i)
            {
                osg::Geometry* geom = mMesh->mOsgGeometries[i];
                osg::Geode* materialGeode = new osg::Geode;
                materialGeode->addDrawable(geom);
                mOsgGroup->addChild(materialGeode);
                setMaterial(i, mMesh->mSubmeshes[i].defaultMaterial);
            }
        }

        void setMaterial(uint32_t index, Material* material)
        {
            if (index < mOsgGroup->getNumChildren())
            {
                mOsgGroup->getChild(index)->setStateSet(material->getOsgStateSet());
            }
        }

    protected:
        osg::ref_ptr<Mesh> mMesh;
        std::vector<osg::ref_ptr<Material>> mMaterials;

        osg::ref_ptr<osg::Group> mOsgGroup;
    };
}
