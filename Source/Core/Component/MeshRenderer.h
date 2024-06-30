#pragma once
#include <Core/Base/Component.h>
#include <Core/Asset/StaticMeshAsset.h>

#include <osg/Geode>
#include <osg/Geometry>
#include <osgUtil/CullVisitor>

#define GBUFFER_MASK        0x00000001
#define SHADOW_CAST_MASK    0x00000002

namespace xxx
{
    class MeshRenderer : public Component
    {
    public:
        MeshRenderer() : _geode(new osg::Geode)
        {
            Group::addChild(_geode);
        }
        virtual ~MeshRenderer() = default;

        void setMesh(StaticMeshAsset* mesh)
        {
            if (_staticMesh)
            {
                _geode->removeDrawables(0, _staticMesh->_submeshes.size());
                _materials.clear();
                _geometries.clear();
            }

            if (!mesh)
                return;

            _staticMesh = mesh;
            uint32_t submeshCount = _staticMesh->_submeshes.size();
            _materials.resize(submeshCount);
            _geometries.resize(submeshCount);

            for (uint32_t i = 0; i < submeshCount; ++i)
            {
                osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
                _geometries[i] = geometry;
                _geode->addDrawable(_geometries[i]);

                // set vertices and indices
                StaticMeshAsset::Submesh& submesh = _staticMesh->_submeshes[i];
                for (auto& vertexAttribute : submesh._vertexAttributes)
                {
                    geometry->setVertexAttribArray(vertexAttribute.first, vertexAttribute.second);
                }
                geometry->addPrimitiveSet(submesh._drawElements);

                // set material
                geometry->setCullCallback(new GeometryCullCallback);
                setMaterial(submesh._previewMaterial, i, true);
            }
        }

        void setMaterial(MaterialAsset* material, uint32_t index, bool usePreviewMaterial = false)
        {
            _materials[index].first = material;
            _materials[index].second = usePreviewMaterial;
            GeometryCullCallback* cullCallback = dynamic_cast<GeometryCullCallback*>(_geometries[index]->getCullCallback());
            osg::StateSet* materialStateSet = new osg::StateSet(*material->_stateSet);
            osg::Program* materialProgram = new osg::Program;
            materialProgram->addShader(material->_shader);
            materialStateSet->setAttribute(materialProgram, osg::StateAttribute::ON);
            cullCallback->setGBufferStateSet(materialStateSet);
            // create shadow cast stateSet and set it;
            //cullCallback->setShadowCastStateSet();
        }

        uint32_t getSubmeshCount()
        {
            return _staticMesh ? _staticMesh->getSubmeshCount() : 0;
        }

        MaterialAsset* getMaterialAsset(uint32_t index)
        {
            return _materials.size() > index ? _materials[index].first : nullptr;
        }



    private:
        osg::ref_ptr<StaticMeshAsset> _staticMesh;
        // first is material asset, second indicates whether to use the preview material
        std::vector<std::pair<osg::ref_ptr<MaterialAsset>, bool>> _materials;
        osg::ref_ptr<osg::Geode> _geode;
        std::vector<osg::ref_ptr<osg::Geometry>> _geometries;

        class GeometryCullCallback : public osg::DrawableCullCallback
        {
            osg::ref_ptr<osg::StateSet> _gbufferStateSet;
            osg::ref_ptr<osg::StateSet> _shadowCastStateSet;
        public:
            GeometryCullCallback() :
                _gbufferStateSet(nullptr),
                _shadowCastStateSet(nullptr) {}

            virtual bool cull(osg::NodeVisitor* nv, osg::Drawable* drawable, osg::RenderInfo* renderInfo) const
            {
                switch (nv->asCullVisitor()->getCurrentCamera()->getCullMask())
                {
                case GBUFFER_MASK:
                    drawable->setStateSet(_gbufferStateSet);
                    break;
                case SHADOW_CAST_MASK:
                    drawable->setStateSet(_shadowCastStateSet);
                    break;
                default:
                    break;
                }
            }

            void setGBufferStateSet(osg::StateSet* gbufferStateSet)
            {
                _gbufferStateSet = gbufferStateSet;
            }

            void setShadowCastStateSet(osg::StateSet* shadowCastStateSet)
            {
                _shadowCastStateSet = shadowCastStateSet;
            }

        };
    };
}
