#include "Mesh.h"
#include <cassert>

#include <osg/Material>
#include <osgUtil/Optimizer>
#include <osgUtil/TangentSpaceGenerator>
#if 1
namespace xxx
{
    class MeshProcessVisitor : public osg::NodeVisitor
    {
    public:
        MeshProcessVisitor() : NodeVisitor(TRAVERSE_ALL_CHILDREN), mGeode(new osg::Geode) {}

        virtual void apply(osg::MatrixTransform& matrixTransform)
        {
            if (mMatrixStack.empty())
                mMatrixStack.push(matrixTransform.getMatrix());
            else
                mMatrixStack.push(mMatrixStack.top() * matrixTransform.getMatrix());
            traverse(matrixTransform);
            mMatrixStack.pop();
        }

        virtual void apply(osg::Geode& geode)
        {
            for (unsigned int i = 0; i < geode.getNumDrawables(); ++i)
            {
                osg::Geometry* geom = dynamic_cast<osg::Geometry*>(geode.getDrawable(i));
                if (geom)
                {
                    mGeode->addDrawable(geom);
                    if (geom->getStateSet())
                    {
                        osg::Material* material = dynamic_cast<osg::Material*>(geom->getStateSet()->getAttribute(osg::StateAttribute::MATERIAL, 0));
                        if (material)
                        {
                            auto findResult = mMaterialStateSetMap.find(material);
                            if (findResult == mMaterialStateSetMap.end())
                                mMaterialStateSetMap.emplace(material, geom->getStateSet());
                            else
                                geom->setStateSet(findResult->second);
                        }
                    }
                }
            }

            if (!mMatrixStack.empty())
            {
                osg::Matrix modelMatrix = mMatrixStack.top();

                osg::Matrix m(modelMatrix);
                m.setTrans(0.0, 0.0, 0.0);

                osg::Matrix matrix;
                matrix.invert(m);

                osg::Matrix normalMatrix(matrix(0, 0), matrix(1, 0), matrix(2, 0), 0,
                                        matrix(0, 1), matrix(1, 1), matrix(2, 1), 0,
                                        matrix(0, 2), matrix(1, 2), matrix(2, 2), 0,
                                        0, 0, 0, 1);
                for (unsigned int i = 0; i < geode.getNumDrawables(); ++i)
                {
                    osg::Geometry* geom = dynamic_cast<osg::Geometry*>(geode.getDrawable(i));
                    if (geom)
                        applyTransform(geom, modelMatrix, normalMatrix);
                }
            }

            // 不再继续遍历Geometry
            // traverse(geode);
            geode.removeDrawables(0, geode.getNumDrawables());
        }

        osg::Geode* getGeode()
        {
            return mGeode;
        }

    private:
        osg::ref_ptr<osg::Geode> mGeode;
        std::stack<osg::Matrix> mMatrixStack;
        std::map<osg::ref_ptr<osg::Material>, osg::ref_ptr<osg::StateSet>> mMaterialStateSetMap;

        static void applyTransform(osg::Geometry* geom, osg::Matrix& modelMatrix, osg::Matrix& normalMatrix)
        {
            osg::Vec3Array* positions = dynamic_cast<osg::Vec3Array*>(geom->getVertexArray());
            if (positions)
                for (osg::Vec3& pos : *positions)
                    pos = pos * modelMatrix;

            osg::Vec3Array* normals = dynamic_cast<osg::Vec3Array*>(geom->getNormalArray());
            if (normals)
            {
                for (osg::Vec3& nor : *normals)
                {
                    nor = nor * normalMatrix;
                    nor.normalize();
                }
            }
        }
    };

    uint32_t getNodeLOD(osg::Node* node)
    {
        const std::string& name = node->getName();
        size_t pos = name.rfind('.');
        if (pos == std::string::npos)
            return 0;
        std::string lodPostfix = name.substr(pos + 1);
        if (lodPostfix.size() > 3 && std::string(lodPostfix.data(), 3) == "LOD")
            return std::stoi(lodPostfix.substr(3));
        return 0;
    }

    void Mesh::parseOsgNodes(uint32_t lod, const std::vector<osg::ref_ptr<osg::Node>>& nodes)
    {
        if (lod >= mOsgLODSubmeshes.size())
            return;

        osg::ref_ptr<osg::Group> group = new osg::Group;
        for (osg::Node* node : nodes)
            group->addChild(node);
        MeshProcessVisitor mpv;
        group->accept(mpv);
        osgUtil::Optimizer optimizer;
        optimizer.optimize(mpv.getGeode(),
            osgUtil::Optimizer::MERGE_GEOMETRY |
            osgUtil::Optimizer::INDEX_MESH
        );
        uint32_t submeshCount = mpv.getGeode()->getNumDrawables();

        mOsgLODSubmeshes[lod].resize(submeshCount);

        static constexpr int TangentVertexAttributeIndex = 6;
        for (uint32_t j = 0; j < submeshCount; ++j)
        {
            osg::ref_ptr<osgUtil::TangentSpaceGenerator> tangentSpaceGenerator = new osgUtil::TangentSpaceGenerator;
            osg::Geometry* geometry = mpv.getGeode()->getDrawable(j)->asGeometry();
            tangentSpaceGenerator->generate(geometry);

            osg::ref_ptr<osg::Geometry> newGeometry = new osg::Geometry;
            newGeometry->setVertexArray(geometry->getVertexArray());
            if (geometry->getColorArray())
            {
                newGeometry->setVertexAttribArray(1, geometry->getColorArray());
                newGeometry->setVertexAttribBinding(1, geometry->getColorArray()->getNumElements() == 1 ? osg::Geometry::BIND_OVERALL : osg::Geometry::BIND_PER_VERTEX);
            }
            if (geometry->getNormalArray())
            {
                newGeometry->setVertexAttribArray(2, geometry->getNormalArray());
                newGeometry->setVertexAttribBinding(2, osg::Geometry::BIND_PER_VERTEX);
            }
            if (tangentSpaceGenerator->getTangentArray())
            {
                newGeometry->setVertexAttribArray(3, tangentSpaceGenerator->getTangentArray());
                newGeometry->setVertexAttribBinding(3, osg::Geometry::BIND_PER_VERTEX);
            }
            for (uint32_t indexOffset = 0; indexOffset < geometry->getNumTexCoordArrays(); ++indexOffset)
            {
                newGeometry->setVertexAttribArray(4 + indexOffset, geometry->getTexCoordArray(indexOffset));
                newGeometry->setVertexAttribBinding(4 + indexOffset, osg::Geometry::BIND_PER_VERTEX);
            }

            newGeometry->addPrimitiveSet(geometry->getPrimitiveSet(0));

            mOsgLODSubmeshes[lod][j] = newGeometry;
        }
    }

    Mesh::Mesh(const std::string& meshPath)
    {
        osg::ref_ptr<osg::Node> meshNode = osgDB::readNodeFile(meshPath);
        osg::Group* group = dynamic_cast<osg::Group*>(meshNode.get());
        assert(group);
        uint32_t childrenCount = group->getNumChildren();

        std::unordered_map<uint32_t, std::vector<osg::ref_ptr<osg::Node>>> lodNodesMap;
        for (uint32_t i = 0; i < childrenCount; ++i)
        {
            osg::Node* lodNode = group->getChild(i);
            uint32_t lod = getNodeLOD(lodNode);
            auto& nodes = lodNodesMap[lod];
            nodes.emplace_back(lodNode);
        }
        mOsgLODSubmeshes.resize(lodNodesMap.size());
        for (const auto& lodNodes : lodNodesMap)
            parseOsgNodes(lodNodes.first, lodNodes.second);

        mDefaultMaterials.resize(mOsgLODSubmeshes[0].size());
        uint32_t lodCount = mOsgLODSubmeshes.size();
        mLODRanges.resize(lodCount);
        for (uint32_t i = 0; i < lodCount; ++i)
            mLODRanges[i] = { i * 100000000, (i + 1) * 100000000 };
    }

    void Mesh::preSave()
    {
        uint32_t lodCount = mOsgLODSubmeshes.size();
        uint32_t submeshCount = mOsgLODSubmeshes[0].size();
        mLODSubmeshViews.resize(lodCount);
        for (uint32_t i = 0; i < lodCount; ++i)
            mLODSubmeshViews[i].resize(submeshCount);

        // evaluate data size
        size_t dataSize = 0;
        for (const OsgGeometries& submeshGeometries : mOsgLODSubmeshes)
        {
            for (const osg::Geometry* geometry : submeshGeometries)
            {
                dataSize += geometry->getVertexArray()->getTotalDataSize();
                for (const auto& vertexAttribArray : geometry->getVertexAttribArrayList())
                {
                    if (vertexAttribArray)
                        dataSize += vertexAttribArray->getTotalDataSize();
                }
                osg::Geometry::DrawElementsList drawElementList;
                geometry->getDrawElementsList(drawElementList);
                dataSize += drawElementList.at(0)->getTotalDataSize();
            }
        }
        mData.resize(dataSize);

        // fill data
        size_t dataOffset = 0;
        for (uint32_t i = 0; i < lodCount; ++i)
        {
            const OsgGeometries& submeshGeometries = mOsgLODSubmeshes[i];
            for (uint32_t j = 0; j < submeshCount; ++j)
            {
                const osg::Geometry* geometry = submeshGeometries[j];

                // positions
                const osg::Array* positionArray = geometry->getVertexArray();
                VertexAttributeView vav;
                vav.location = 0;
                vav.dimension = positionArray->getDataSize();
                vav.type = positionArray->getDataType();
                vav.offset = dataOffset;
                vav.size = positionArray->getTotalDataSize();
                mLODSubmeshViews[i][j].vertexAttributeViews.emplace_back(vav);
                std::memcpy(mData.data() + dataOffset, positionArray->getDataPointer(), positionArray->getTotalDataSize());
                dataOffset += positionArray->getTotalDataSize();

                // other vertex attributes
                const osg::Geometry::ArrayList& vertexAttribArrayList = geometry->getVertexAttribArrayList();
                for (uint32_t k = 0; k < vertexAttribArrayList.size(); ++k)
                {
                    if (!vertexAttribArrayList[k])
                        continue;

                    const osg::Array* vertexAttribArray = vertexAttribArrayList[k];
                    VertexAttributeView vav;
                    vav.location = k;
                    vav.dimension = vertexAttribArray->getDataSize();
                    vav.type = vertexAttribArray->getDataType();
                    vav.offset = dataOffset;
                    vav.size = vertexAttribArray->getTotalDataSize();
                    mLODSubmeshViews[i][j].vertexAttributeViews.emplace_back(vav);
                    std::memcpy(mData.data() + dataOffset, vertexAttribArray->getDataPointer(), vertexAttribArray->getTotalDataSize());
                    dataOffset += vertexAttribArray->getTotalDataSize();
                }

                osg::Geometry::DrawElementsList drawElementList;
                geometry->getDrawElementsList(drawElementList);
                osg::DrawElements* drawElements = drawElementList.at(0);
                IndexBufferView& ibv = mLODSubmeshViews[i][j].indexBufferView;
                ibv.type = drawElements->getDataType();
                ibv.offset = dataOffset;
                ibv.size = drawElements->getTotalDataSize();
                std::memcpy(mData.data() + dataOffset, drawElements->getDataPointer(), drawElements->getTotalDataSize());
                dataOffset += drawElements->getTotalDataSize();
            }
        }
        compressData();
    }

    void Mesh::postSave()
    {
        mData.clear();
        mData.shrink_to_fit();
        mLODSubmeshViews.clear();
        mLODSubmeshViews.shrink_to_fit();
    }

    void Mesh::postLoad()
    {
        decompressData();
        uint32_t lodCount = mLODSubmeshViews.size();
        uint32_t submeshCount = mLODSubmeshViews[0].size();
        mOsgLODSubmeshes.resize(lodCount);
        for (uint32_t i = 0; i < lodCount; ++i)
            mOsgLODSubmeshes[i].resize(submeshCount);

        for (uint32_t i = 0; i < lodCount; ++i)
        {
            const std::vector<SubmeshView>& submeshViews = mLODSubmeshViews[i];
            for (uint32_t j = 0; j < submeshCount; ++j)
            {
                const SubmeshView& submeshView = submeshViews[j];
                osg::Geometry* geometry = new osg::Geometry;
                for (const VertexAttributeView& vav : submeshView.vertexAttributeViews)
                {
                    osg::Array* osgArray = createOsgArrayByVertexAttributeView(vav);
                    if (osgArray->getNumElements() == 1)
                        osgArray->setBinding(osg::Array::BIND_OVERALL);
                    else
                        osgArray->setBinding(osg::Array::BIND_PER_VERTEX);

                    if (vav.location == 0)
                        geometry->setVertexArray(osgArray);
                    else
                        geometry->setVertexAttribArray(vav.location, osgArray);
                }

                osg::DrawElements* drawElements = createOsgDrawElementsByIndexBufferView(submeshView.indexBufferView);
                geometry->addPrimitiveSet(drawElements);

                mOsgLODSubmeshes[i][j] = geometry;
            }
        }
        mData.clear();
        mData.shrink_to_fit();
        mLODSubmeshViews.clear();
        mLODSubmeshViews.shrink_to_fit();
    }

    osg::Array* Mesh::createOsgArrayByVertexAttributeView(const VertexAttributeView& vav)
    {
        uint8_t* data = mData.data() + vav.offset;
        switch (vav.type)
        {
        case GL_BYTE:
        {
            switch (vav.dimension)
            {
            case 1: return new osg::ByteArray(vav.size / sizeof(GLbyte), (GLbyte*)(data));
            case 2: return new osg::Vec2bArray(vav.size / sizeof(osg::Vec2b), (osg::Vec2b*)(data));
            case 3: return new osg::Vec3bArray(vav.size / sizeof(osg::Vec3b), (osg::Vec3b*)(data));
            case 4: return new osg::Vec4bArray(vav.size / sizeof(osg::Vec4b), (osg::Vec4b*)(data));
            }
            break;
        }
        case GL_UNSIGNED_BYTE:
        {
            switch (vav.dimension)
            {
            case 1: return new osg::UByteArray(vav.size / sizeof(GLubyte), (GLubyte*)(data));
            case 2: return new osg::Vec2ubArray(vav.size / sizeof(osg::Vec2ub), (osg::Vec2ub*)(data));
            case 3: return new osg::Vec3ubArray(vav.size / sizeof(osg::Vec3ub), (osg::Vec3ub*)(data));
            case 4: return new osg::Vec4ubArray(vav.size / sizeof(osg::Vec4ub), (osg::Vec4ub*)(data));
            }
            break;
        }
        case GL_SHORT:
        {
            switch (vav.dimension)
            {
            case 1: return new osg::ShortArray(vav.size / sizeof(GLshort), (GLshort*)(data));
            case 2: return new osg::Vec2sArray(vav.size / sizeof(osg::Vec2s), (osg::Vec2s*)(data));
            case 3: return new osg::Vec3sArray(vav.size / sizeof(osg::Vec3s), (osg::Vec3s*)(data));
            case 4: return new osg::Vec4sArray(vav.size / sizeof(osg::Vec4s), (osg::Vec4s*)(data));
            }
            break;
        }
        case GL_UNSIGNED_SHORT:
        {
            switch (vav.dimension)
            {
            case 1: return new osg::UShortArray(vav.size / sizeof(GLushort), (GLushort*)(data));
            case 2: return new osg::Vec2usArray(vav.size / sizeof(osg::Vec2us), (osg::Vec2us*)(data));
            case 3: return new osg::Vec3usArray(vav.size / sizeof(osg::Vec3us), (osg::Vec3us*)(data));
            case 4: return new osg::Vec4usArray(vav.size / sizeof(osg::Vec4us), (osg::Vec4us*)(data));
            }
            break;
        }
        case GL_INT:
        {
            switch (vav.dimension)
            {
            case 1: return new osg::IntArray(vav.size / sizeof(GLint), (GLint*)(data));
            case 2: return new osg::Vec2iArray(vav.size / sizeof(osg::Vec2i), (osg::Vec2i*)(data));
            case 3: return new osg::Vec3iArray(vav.size / sizeof(osg::Vec3i), (osg::Vec3i*)(data));
            case 4: return new osg::Vec4iArray(vav.size / sizeof(osg::Vec4i), (osg::Vec4i*)(data));
            }
            break;
        }
        case GL_UNSIGNED_INT:
        {
            switch (vav.dimension)
            {
            case 1: return new osg::UIntArray(vav.size / sizeof(GLuint), (GLuint*)(data));
            case 2: return new osg::Vec2uiArray(vav.size / sizeof(osg::Vec2ui), (osg::Vec2ui*)(data));
            case 3: return new osg::Vec3uiArray(vav.size / sizeof(osg::Vec3ui), (osg::Vec3ui*)(data));
            case 4: return new osg::Vec4uiArray(vav.size / sizeof(osg::Vec4ui), (osg::Vec4ui*)(data));
            }
            break;
        }
        case GL_FLOAT:
        {
            switch (vav.dimension)
            {
            case 1: return new osg::FloatArray(vav.size / sizeof(GLfloat), (GLfloat*)(data));
            case 2: return new osg::Vec2Array(vav.size / sizeof(osg::Vec2f), (osg::Vec2f*)(data));
            case 3: return new osg::Vec3Array(vav.size / sizeof(osg::Vec3f), (osg::Vec3f*)(data));
            case 4: return new osg::Vec4Array(vav.size / sizeof(osg::Vec4f), (osg::Vec4f*)(data));
            }
            break;
        }
        case GL_DOUBLE:
        {
            switch (vav.dimension)
            {
            case 1: return new osg::DoubleArray(vav.size / sizeof(GLdouble), (GLdouble*)(data));
            case 2: return new osg::Vec2dArray(vav.size / sizeof(osg::Vec2d), (osg::Vec2d*)(data));
            case 3: return new osg::Vec3dArray(vav.size / sizeof(osg::Vec3d), (osg::Vec3d*)(data));
            case 4: return new osg::Vec4dArray(vav.size / sizeof(osg::Vec4d), (osg::Vec4d*)(data));
            }
            break;
        }
        default:
            break;
        }
        return nullptr;
    }

    osg::DrawElements* Mesh::createOsgDrawElementsByIndexBufferView(const IndexBufferView& ibv)
    {
        uint8_t* data = mData.data() + ibv.offset;
        switch (ibv.type)
        {
        case GL_UNSIGNED_BYTE: return new osg::DrawElementsUByte(GL_TRIANGLES, ibv.size / sizeof(GLubyte), (GLubyte*)(data));
        case GL_UNSIGNED_SHORT: return new osg::DrawElementsUShort(GL_TRIANGLES, ibv.size / sizeof(GLushort), (GLushort*)(data));
        case GL_UNSIGNED_INT: return new osg::DrawElementsUInt(GL_TRIANGLES, ibv.size / sizeof(GLuint), (GLuint*)(data));
        default:
            return nullptr;
        }
    }

    namespace refl
    {
        template <> Type* Reflection::createType<VertexAttributeView>()
        {
            Structure* structure = new TStructure<VertexAttributeView>("VertexAttributeView");
            structure->addProperty("Location", &VertexAttributeView::location);
            structure->addProperty("Dimension", &VertexAttributeView::dimension);
            structure->addProperty("Type", &VertexAttributeView::type);
            structure->addProperty("Offset", &VertexAttributeView::offset);
            structure->addProperty("Size", &VertexAttributeView::size);
            return structure;
        }

        template <> Type* Reflection::createType<IndexBufferView>()
        {
            Structure* structure = new TStructure<IndexBufferView>("IndexBufferView");
            structure->addProperty("Type", &IndexBufferView::type);
            structure->addProperty("Offset", &IndexBufferView::offset);
            structure->addProperty("Size", &IndexBufferView::size);
            return structure;
        }

        template <> Type* Reflection::createType<SubmeshView>()
        {
            Structure* structure = new TStructure<SubmeshView>("SubmeshView");
            structure->addProperty("VertexAttributeViews", &SubmeshView::vertexAttributeViews);
            structure->addProperty("IndexBufferView", &SubmeshView::indexBufferView);
            return structure;
        }

        template <> Type* Reflection::createType<Mesh>()
        {
            Class* clazz = new TClass<Mesh>("Mesh");
            clazz->addProperty("Data", &Mesh::mData);
            clazz->addProperty("LODSubmeshViews", &Mesh::mLODSubmeshViews);
            clazz->addProperty("LODRanges", &Mesh::mLODRanges);
            clazz->addProperty("DefaultMaterials", &Mesh::mDefaultMaterials);
            clazz->addProperty("DataCompression", &Mesh::mDataCompression);
            return clazz;
        }
    }
}
#endif // 1

