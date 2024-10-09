#pragma once
#include "Material.h"

namespace xxx
{
    struct VertexAttributeView
    {
        uint32_t location;
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

    struct SubmeshView
    {
        std::vector<VertexAttributeView> vertexAttributeViews;
        IndexBufferView indexBufferView;
        osg::ref_ptr<Material> defaultMaterial;
    };

    class MeshRenderer;
    class Mesh : public Object
    {
        friend class MeshRenderer;
        REFLECT_CLASS(Mesh)
    public:
        Mesh() = default;
        Mesh(const std::string& meshPath);

        virtual void preSerialize(Serializer* serializer) override;

        virtual void postSerialize(Serializer* serializer) override;

        std::vector<osg::Geometry*> generateGeometries()
        {
            std::vector<osg::Geometry*> geometries;
            for (OsgGeometryData& geomData : mOsgGeometryDatas)
            {
                osg::Geometry* geometry = new osg::Geometry;
                for (auto& vertexAttribute : geomData.vertexAttributes)
                {
                    if (vertexAttribute.first == 0)
                        geometry->setVertexArray(vertexAttribute.second);
                    else if (vertexAttribute.first == 2)
                        geometry->setNormalArray(vertexAttribute.second);
                    else if (vertexAttribute.first == 3)
                        geometry->setColorArray(vertexAttribute.second);
                    else if (vertexAttribute.first >= 8 && vertexAttribute.first < 12)
                        geometry->setTexCoordArray(vertexAttribute.first - 8, vertexAttribute.second);
                    else
                        geometry->setVertexAttribArray(vertexAttribute.first, vertexAttribute.second);
                }
                geometry->addPrimitiveSet(geomData.drawElements);
                geometries.push_back(geometry);
            }
            return geometries;
        }

    protected:
        std::vector<uint8_t> mData;
        std::vector<SubmeshView> mSubmeshViews;

        struct OsgGeometryData
        {
            std::vector<std::pair<uint32_t, osg::ref_ptr<osg::Array>>> vertexAttributes;
            osg::ref_ptr<osg::DrawElements> drawElements;
        };
        std::vector<OsgGeometryData> mOsgGeometryDatas;

        static osg::Array* createOsgArrayByVertexAttributeView(VertexAttributeView& vav, uint8_t* data);
        static osg::DrawElements* createOsgDrawElementsByIndexBufferView(IndexBufferView& ibv, uint8_t* data);
    };

    namespace refl
    {
        template <> inline Type* Reflection::createType<VertexAttributeView>()
        {
            Struct* structure = new StructInstance<VertexAttributeView>("VertexAttributeView");
            structure->addProperty("Location", &VertexAttributeView::location);
            structure->addProperty("Dimension", &VertexAttributeView::dimension);
            structure->addProperty("Type", &VertexAttributeView::type);
            structure->addProperty("Offset", &VertexAttributeView::offset);
            structure->addProperty("Size", &VertexAttributeView::size);
            return structure;
        }

        template <> inline Type* Reflection::createType<IndexBufferView>()
        {
            Struct* structure = new StructInstance<IndexBufferView>("IndexBufferView");
            structure->addProperty("Type", &IndexBufferView::type);
            structure->addProperty("Offset", &IndexBufferView::offset);
            structure->addProperty("Size", &IndexBufferView::size);
            return structure;
        }

        template <> inline Type* Reflection::createType<SubmeshView>()
        {
            Struct* structure = new StructInstance<SubmeshView>("SubmeshView");
            structure->addProperty("VertexAttributeViews", &SubmeshView::vertexAttributeViews);
            structure->addProperty("IndexBufferView", &SubmeshView::indexBufferView);
            structure->addProperty("DefaultMaterial", &SubmeshView::defaultMaterial);
            return structure;
        }

        template <> inline Type* Reflection::createType<Mesh>()
        {
            Class* clazz = new ClassInstance<Mesh>("Mesh");
            clazz->addProperty("Data", &Mesh::mData);
            clazz->addProperty("SubmeshViews", &Mesh::mSubmeshViews);
            getClassMap().emplace("Mesh", clazz);
            return clazz;
        }
    }
}
