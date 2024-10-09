#include "Mesh.h"

#include <osg/MatrixTransform>
#include <osg/Material>
#include <osgDB/ReadFile>
#include <osgUtil/Optimizer>
#include <osgUtil/MeshOptimizers>
#include <osgUtil/TangentSpaceGenerator>

#include <stack>

namespace xxx
{
    class MeshOptimizeVisitor : public osg::NodeVisitor
    {
    public:
        MeshOptimizeVisitor() : NodeVisitor(TRAVERSE_ALL_CHILDREN), _geode(new osg::Geode) {}

        virtual void apply(osg::MatrixTransform& matrixTransform)
        {
            if (_matrixStack.empty())
                _matrixStack.push(matrixTransform.getMatrix());
            else
                _matrixStack.push(_matrixStack.top() * matrixTransform.getMatrix());
            traverse(matrixTransform);
            _matrixStack.pop();
        }

        virtual void apply(osg::Geode& geode)
        {
            osg::ref_ptr<osgUtil::TangentSpaceGenerator> tangentSpaceGenerator = new osgUtil::TangentSpaceGenerator;

            for (unsigned int i = 0; i < geode.getNumDrawables(); ++i)
            {
                osg::Geometry* geom = dynamic_cast<osg::Geometry*>(geode.getDrawable(i));
                if (geom)
                {
                    _geode->addDrawable(geom);

                    tangentSpaceGenerator->generate(geom);
                    geom->setVertexAttribArray(_tangentVertexAttributeIndex, tangentSpaceGenerator->getTangentArray());
                    geom->setVertexAttribBinding(_tangentVertexAttributeIndex, osg::Geometry::BIND_PER_VERTEX);

                    if (geom->getStateSet())
                    {
                        osg::Material* material = dynamic_cast<osg::Material*>(geom->getStateSet()->getAttribute(osg::StateAttribute::MATERIAL, 0));
                        if (material)
                        {
                            auto findResult = _materialStateSetMap.find(material);
                            if (findResult == _materialStateSetMap.end())
                            {
                                _materialStateSetMap.emplace(material, geom->getStateSet());
                            }
                            else
                            {
                                geom->setStateSet(findResult->second);
                            }
                        }
                    }
                }
            }

            if (!_matrixStack.empty())
            {
                osg::Matrix modelMatrix = _matrixStack.top();

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
            {
                for (osg::Vec3& nor : *normals)
                {
                    nor = nor * normalMatrix;
                    nor.normalize();
                }
            }

            osg::Vec4Array* tangents = dynamic_cast<osg::Vec4Array*>(geom->getVertexAttribArray(_tangentVertexAttributeIndex));
            if (tangents)
            {
                for (osg::Vec4& tan : *tangents)
                {
                    osg::Vec3 tanTemp = osg::Vec3(tan.x(), tan.y(), tan.z()) * normalMatrix;
                    tanTemp.normalize();
                    tan = osg::Vec4(tanTemp, tan.w());
                }
            }
        }

        std::stack<osg::Matrix> _matrixStack;
        osg::ref_ptr<osg::Geode> _geode;

        std::map<osg::ref_ptr<osg::Material>, osg::ref_ptr<osg::StateSet>> _materialStateSetMap;

        static const uint32_t _tangentVertexAttributeIndex = 6;
    };

    Mesh::Mesh(const std::string& meshPath)
    {
        osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(meshPath);
        MeshOptimizeVisitor mov;
        node->accept(mov);
        osgUtil::Optimizer optimizer;
        optimizer.optimize(mov.getGeode(),
            osgUtil::Optimizer::MERGE_GEOMETRY |
            osgUtil::Optimizer::INDEX_MESH |
            osgUtil::Optimizer::VERTEX_POSTTRANSFORM |
            osgUtil::Optimizer::VERTEX_PRETRANSFORM
        );

        for (uint32_t i = 0; i < mov.getGeode()->getNumDrawables(); ++i)
        {
            osg::Geometry* geom = dynamic_cast<osg::Geometry*>(mov.getGeode()->getDrawable(i));
            if (geom)
            {
                OsgGeometryData osgGeometryData;
                osgGeometryData.vertexAttributes.emplace_back(0, geom->getVertexArray());
                if (geom->getNormalArray())
                    osgGeometryData.vertexAttributes.emplace_back(2, geom->getNormalArray());
                if (geom->getColorArray())
                    osgGeometryData.vertexAttributes.emplace_back(3, geom->getColorArray());
                uint32_t texcoordIndex = 8;
                for (osg::Array* texcoordArray : geom->getTexCoordArrayList())
                {
                    if (texcoordArray)
                        osgGeometryData.vertexAttributes.emplace_back(texcoordIndex, texcoordArray);
                    ++texcoordIndex;
                }
                uint32_t vertexAttributeIndex = 0;
                for (osg::Array* vertexAttributeArray : geom->getVertexAttribArrayList())
                {
                    if (vertexAttributeArray)
                        osgGeometryData.vertexAttributes.emplace_back(vertexAttributeIndex, vertexAttributeArray);
                    ++vertexAttributeIndex;
                }

                // fixed only use PrimitiveSet(0), so that we can assign an IndexBufferView to each Geometry
                osgGeometryData.drawElements = dynamic_cast<osg::DrawElements*>(geom->getPrimitiveSet(0));
                mOsgGeometryDatas.emplace_back(osgGeometryData);
            }
        }
        return;
    }

    void Mesh::preSerialize(Serializer* serializer)
    {
        if (serializer->isSaver())
        {
            mSubmeshViews.resize(mOsgGeometryDatas.size());

            size_t dataSize = 0;
            for (OsgGeometryData& geomData : mOsgGeometryDatas)
            {
                for (auto& vertexAttribute : geomData.vertexAttributes)
                    dataSize += vertexAttribute.second->getTotalDataSize();

                dataSize += geomData.drawElements->getTotalDataSize();
            }
            mData.resize(dataSize);

            size_t dataOffset = 0;
            for (uint32_t i = 0; i < mOsgGeometryDatas.size(); ++i)
            {
                OsgGeometryData& geomData = mOsgGeometryDatas[i];

                for (auto& vertexAttribute : geomData.vertexAttributes)
                {
                    VertexAttributeView vav;
                    osg::Array* vertexAttributeArray = vertexAttribute.second;
                    vav.location = vertexAttribute.first;
                    vav.dimension = vertexAttributeArray->getDataSize();
                    vav.type = vertexAttributeArray->getDataType();
                    vav.offset = dataOffset;
                    vav.size = vertexAttributeArray->getTotalDataSize();
                    mSubmeshViews[i].vertexAttributeViews.emplace_back(vav);

                    std::memcpy(mData.data() + dataOffset, vertexAttributeArray->getDataPointer(), vertexAttributeArray->getTotalDataSize());
                    dataOffset += vertexAttributeArray->getTotalDataSize();
                }

                osg::DrawElements* drawElements = geomData.drawElements;
                IndexBufferView& ibv = mSubmeshViews[i].indexBufferView;
                ibv.type = drawElements->getDataType();
                ibv.offset = dataOffset;
                ibv.size = drawElements->getTotalDataSize();

                std::memcpy(mData.data() + dataOffset, drawElements->getDataPointer(), drawElements->getTotalDataSize());
                dataOffset += drawElements->getTotalDataSize();
            }
        }
    }

    void Mesh::postSerialize(Serializer* serializer)
    {
        if (serializer->isLoader())
        {
            for (SubmeshView& submeshView : mSubmeshViews)
            {
                OsgGeometryData geomData;

                for (VertexAttributeView& vav : submeshView.vertexAttributeViews)
                {
                    osg::Array* osgArray = createOsgArrayByVertexAttributeView(vav, mData.data());
                    //if (osgArray->getNumElements() == 1)
                    //    osgArray->setBinding(osg::Array::BIND_OVERALL);
                    geomData.vertexAttributes.emplace_back(vav.location, osgArray);
                }

                geomData.drawElements = createOsgDrawElementsByIndexBufferView(submeshView.indexBufferView, mData.data());

                mOsgGeometryDatas.emplace_back(geomData);
            }
        }
        mData.clear();
        mSubmeshViews.clear();
    }

    osg::Array* Mesh::createOsgArrayByVertexAttributeView(VertexAttributeView& vav, uint8_t* data)
    {
        switch (vav.type)
        {
        case GL_BYTE:
        {
            switch (vav.dimension)
            {
            case 1: return new osg::ByteArray(vav.size / sizeof(GLbyte), (GLbyte*)(data + vav.offset));
            case 2: return new osg::Vec2bArray(vav.size / sizeof(osg::Vec2b), (osg::Vec2b*)(data + vav.offset));
            case 3: return new osg::Vec3bArray(vav.size / sizeof(osg::Vec3b), (osg::Vec3b*)(data + vav.offset));
            case 4: return new osg::Vec4bArray(vav.size / sizeof(osg::Vec4b), (osg::Vec4b*)(data + vav.offset));
            }
            break;
        }
        case GL_UNSIGNED_BYTE:
        {
            switch (vav.dimension)
            {
            case 1: return new osg::UByteArray(vav.size / sizeof(GLubyte), (GLubyte*)(data + vav.offset));
            case 2: return new osg::Vec2ubArray(vav.size / sizeof(osg::Vec2ub), (osg::Vec2ub*)(data + vav.offset));
            case 3: return new osg::Vec3ubArray(vav.size / sizeof(osg::Vec3ub), (osg::Vec3ub*)(data + vav.offset));
            case 4: return new osg::Vec4ubArray(vav.size / sizeof(osg::Vec4ub), (osg::Vec4ub*)(data + vav.offset));
            }
            break;
        }
        case GL_SHORT:
        {
            switch (vav.dimension)
            {
            case 1: return new osg::ShortArray(vav.size / sizeof(GLshort), (GLshort*)(data + vav.offset));
            case 2: return new osg::Vec2sArray(vav.size / sizeof(osg::Vec2s), (osg::Vec2s*)(data + vav.offset));
            case 3: return new osg::Vec3sArray(vav.size / sizeof(osg::Vec3s), (osg::Vec3s*)(data + vav.offset));
            case 4: return new osg::Vec4sArray(vav.size / sizeof(osg::Vec4s), (osg::Vec4s*)(data + vav.offset));
            }
            break;
        }
        case GL_UNSIGNED_SHORT:
        {
            switch (vav.dimension)
            {
            case 1: return new osg::UShortArray(vav.size / sizeof(GLushort), (GLushort*)(data + vav.offset));
            case 2: return new osg::Vec2usArray(vav.size / sizeof(osg::Vec2us), (osg::Vec2us*)(data + vav.offset));
            case 3: return new osg::Vec3usArray(vav.size / sizeof(osg::Vec3us), (osg::Vec3us*)(data + vav.offset));
            case 4: return new osg::Vec4usArray(vav.size / sizeof(osg::Vec4us), (osg::Vec4us*)(data + vav.offset));
            }
            break;
        }
        case GL_INT:
        {
            switch (vav.dimension)
            {
            case 1: return new osg::IntArray(vav.size / sizeof(GLint), (GLint*)(data + vav.offset));
            case 2: return new osg::Vec2iArray(vav.size / sizeof(osg::Vec2i), (osg::Vec2i*)(data + vav.offset));
            case 3: return new osg::Vec3iArray(vav.size / sizeof(osg::Vec3i), (osg::Vec3i*)(data + vav.offset));
            case 4: return new osg::Vec4iArray(vav.size / sizeof(osg::Vec4i), (osg::Vec4i*)(data + vav.offset));
            }
            break;
        }
        case GL_UNSIGNED_INT:
        {
            switch (vav.dimension)
            {
            case 1: return new osg::UIntArray(vav.size / sizeof(GLuint), (GLuint*)(data + vav.offset));
            case 2: return new osg::Vec2uiArray(vav.size / sizeof(osg::Vec2ui), (osg::Vec2ui*)(data + vav.offset));
            case 3: return new osg::Vec3uiArray(vav.size / sizeof(osg::Vec3ui), (osg::Vec3ui*)(data + vav.offset));
            case 4: return new osg::Vec4uiArray(vav.size / sizeof(osg::Vec4ui), (osg::Vec4ui*)(data + vav.offset));
            }
            break;
        }
        case GL_FLOAT:
        {
            switch (vav.dimension)
            {
            case 1: return new osg::FloatArray(vav.size / sizeof(GLfloat), (GLfloat*)(data + vav.offset));
            case 2: return new osg::Vec2Array(vav.size / sizeof(osg::Vec2f), (osg::Vec2f*)(data + vav.offset));
            case 3: return new osg::Vec3Array(vav.size / sizeof(osg::Vec3f), (osg::Vec3f*)(data + vav.offset));
            case 4: return new osg::Vec4Array(vav.size / sizeof(osg::Vec4f), (osg::Vec4f*)(data + vav.offset));
            }
            break;
        }
        case GL_DOUBLE:
        {
            switch (vav.dimension)
            {
            case 1: return new osg::DoubleArray(vav.size / sizeof(GLdouble), (GLdouble*)(data + vav.offset));
            case 2: return new osg::Vec2dArray(vav.size / sizeof(osg::Vec2d), (osg::Vec2d*)(data + vav.offset));
            case 3: return new osg::Vec3dArray(vav.size / sizeof(osg::Vec3d), (osg::Vec3d*)(data + vav.offset));
            case 4: return new osg::Vec4dArray(vav.size / sizeof(osg::Vec4d), (osg::Vec4d*)(data + vav.offset));
            }
            break;
        }
        default:
            break;
        }
        return nullptr;
    }

    osg::DrawElements* Mesh::createOsgDrawElementsByIndexBufferView(IndexBufferView& ibv, uint8_t* data)
    {
        switch (ibv.type)
        {
        case GL_UNSIGNED_BYTE: return new osg::DrawElementsUByte(GL_TRIANGLES, ibv.size / sizeof(GLubyte), (GLubyte*)(data + ibv.offset));
        case GL_UNSIGNED_SHORT: return new osg::DrawElementsUShort(GL_TRIANGLES, ibv.size / sizeof(GLushort), (GLushort*)(data + ibv.offset));
        case GL_UNSIGNED_INT: return new osg::DrawElementsUInt(GL_TRIANGLES, ibv.size / sizeof(GLuint), (GLuint*)(data + ibv.offset));
        default:
            return nullptr;
        }
    }
}
