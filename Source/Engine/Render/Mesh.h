#pragma once
#include "Material.h"

#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgUtil/Optimizer>
#include <osgUtil/MeshOptimizers>
#include <osgUtil/TangentSpaceGenerator>

#include <stack>

namespace xxx
{
    struct VertexAttributeView
    {
        uint8_t dimension;
        GLenum type;
        size_t offset;
        size_t size;
    };

    struct IndexBufferView
    {
        GLenum type;
        size_t offset;
        size_t size;
    };

    class MeshOptimizeVisitor : public osg::NodeVisitor
    {
    public:
        MeshOptimizeVisitor() : NodeVisitor(TRAVERSE_ALL_CHILDREN), _geode(new osg::Geode)
        {
            _matrixStack.push(osg::Matrix::identity());
        }

        virtual void apply(osg::MatrixTransform& matrixTransform)
        {
            _matrixStack.push(_matrixStack.top() * matrixTransform.getMatrix());
            traverse(matrixTransform);
            _matrixStack.pop();
        }

        virtual void apply(osg::Geode& geode)
        {
            osg::Matrix modelMatrix = _matrixStack.top();
            osg::Matrix inverseModelMatrix = osg::Matrix::inverse(modelMatrix);
            osg::Matrix normalMatrix;
            normalMatrix.transpose(inverseModelMatrix);

            osg::ref_ptr<osgUtil::TangentSpaceGenerator> tangentSpaceGenerator = new osgUtil::TangentSpaceGenerator;

            for (unsigned int i = 0; i < geode.getNumDrawables(); ++i)
            {
                osg::Geometry* geom = dynamic_cast<osg::Geometry*>(geode.getDrawable(i));
                if (geom)
                {
                    tangentSpaceGenerator->generate(geom);
                    geom->setVertexAttribArray(6, tangentSpaceGenerator->getTangentArray());
                    geom->setVertexAttribBinding(6, osg::Geometry::BIND_PER_VERTEX);

                    applyTransform(geom, modelMatrix, normalMatrix);
                    _geometries.emplace_back(geom);
                    _geode->addDrawable(geom);
                }
            }
            // 不再继续遍历Geometry
            // traverse(geode);
            geode.removeDrawables(0, geode.getNumDrawables());
        }

        const auto& getGeometries() const
        {
            return _geometries;
        }

        osg::Geode* getGeode()
        {
            return _geode;
        }

    protected:
        static void applyTransform(osg::Geometry* geom, osg::Matrix& modelMatrix, osg::Matrix& normalMatrix)
        {
            osg::Vec3Array* positions = dynamic_cast<osg::Vec3Array*>(geom->getVertexArray());
            if (positions)
                for (osg::Vec3& pos : *positions)
                    pos = pos * modelMatrix;

            osg::Vec3Array* normals = dynamic_cast<osg::Vec3Array*>(geom->getNormalArray());
            if (normals)
                for (osg::Vec3& nor : *normals)
                    nor = nor * normalMatrix;

            osg::Vec4Array* tangents = dynamic_cast<osg::Vec4Array*>(geom->getVertexAttribArray(6));
            if (tangents)
                for (osg::Vec4& tan : *tangents)
                    tan = osg::Vec4(osg::Vec3(tan.x(), tan.y(), tan.z()) * normalMatrix, tan.w());
        }

        std::stack<osg::Matrix> _matrixStack;
        std::vector<osg::ref_ptr<osg::Geometry>> _geometries;
        osg::ref_ptr<osg::Geode> _geode;
    };

    class Mesh : public Object
    {
        REFLECT_CLASS(Mesh)
    public:
        Mesh() = default;
        Mesh(const std::string& meshPath)
        {
            osg::Node* node = osgDB::readNodeFile(meshPath);
            MeshOptimizeVisitor tv;
            node->accept(tv);
            mOsgGeometries = tv.getGeometries();
            osgUtil::Optimizer optimizer;
            optimizer.optimize(tv.getGeode(), osgUtil::Optimizer::MERGE_GEOMETRY | osgUtil::Optimizer::INDEX_MESH);
            return;
        }

        virtual void preSerialize() override
        {
            // geometries -> data
        }

        virtual void postSerialize() override
        {
            // data -> geometries
        }

    protected:
        std::vector<uint8_t> mData;
        std::vector<std::pair<uint32_t, VertexAttributeView>> mVertexAttributeViews;
        std::vector<IndexBufferView> mIndexBufferViews;

        std::vector<osg::ref_ptr<osg::Geometry>> mOsgGeometries;
    };

    namespace refl
    {
        template <> inline Type* Reflection::getType<VertexAttributeView>()
        {
            Struct* structure = new StructInstance<VertexAttributeView>("VertexAttributeView");
            structure->addProperty("Dimension", &VertexAttributeView::dimension);
            structure->addProperty("Type", &VertexAttributeView::type);
            structure->addProperty("Offset", &VertexAttributeView::offset);
            structure->addProperty("Size", &VertexAttributeView::size);
            return structure;
        }

        template <> inline Type* Reflection::getType<IndexBufferView>()
        {
            Struct* structure = new StructInstance<IndexBufferView>("IndexBufferView");
            structure->addProperty("Type", &IndexBufferView::type);
            structure->addProperty("Offset", &IndexBufferView::offset);
            structure->addProperty("Size", &IndexBufferView::size);
            return structure;
        }

        template <> inline Type* Reflection::getType<Mesh>()
        {
            Class* clazz = new ClassInstance<Mesh>("Mesh");
            clazz->addProperty("Data", &Mesh::mData);
            clazz->addProperty("VertexAttributeViews", &Mesh::mVertexAttributeViews);
            clazz->addProperty("IndexBufferViews", &Mesh::mIndexBufferViews);
            return clazz;
        }
    }
}
