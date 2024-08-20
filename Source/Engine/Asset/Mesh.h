#pragma once
#include "Material.h"
#include <osg/Geometry>

namespace xxx
{
    class AStaticMesh : public Asset
    {
        friend class CMeshRenderer;
    public:
        AStaticMesh() : Asset(Type::StaticMesh) {}
        virtual ~AStaticMesh() = default;

        virtual void serialize(Json& json, std::vector<char>& binary, std::vector<std::string>& reference) const override
        {
            size_t binarySize = 0;
            for (const auto& submesh : _submeshes)
            {
                for (const auto& vertexAttribute : submesh.vertexAttributes)
                    binarySize += vertexAttribute.second->getTotalDataSize();
                binarySize += submesh.indexBuffer->getTotalDataSize();
            }
            binary.resize(binarySize);

            size_t bufferOffset = 0;
            std::vector<Json> submeshesJsonArray;
            for (const auto& submesh : _submeshes)
            {
                std::vector<Json> vertexAttributesJsonArray;
                for (const auto& vertexAttribute : submesh.vertexAttributes)
                {
                    Json vertexAttributeJson;
                    size_t bufferSize = vertexAttribute.second->getTotalDataSize();
                    vertexAttributeJson["Index"] = vertexAttribute.first;
                    vertexAttributeJson["Type"] = _sArrayTypeStringMap.forwardAt(vertexAttribute.second->getType());
                    vertexAttributeJson["Offset"] = bufferOffset;
                    vertexAttributeJson["Size"] = bufferSize;
                    vertexAttributesJsonArray.emplace_back(vertexAttributeJson);
                    std::memcpy(
                        &binary[bufferOffset],
                        (char*)vertexAttribute.second->getDataPointer(),
                        bufferSize
                    );
                    bufferOffset += bufferSize;
                }

                Json indexBufferJson;
                {
                    size_t bufferSize = submesh.indexBuffer->getTotalDataSize();
                    indexBufferJson["Type"] = _sIndexTypeStringMap.forwardAt(submesh.indexBuffer->getType());
                    indexBufferJson["Offset"] = bufferOffset;
                    indexBufferJson["Size"] = bufferSize;
                    std::memcpy(
                        &binary[bufferOffset],
                        (char*)submesh.indexBuffer->getDataPointer(),
                        bufferSize
                    );
                    bufferOffset += bufferSize;
                }

                Json submeshJson;
                submeshJson["VertexAttributes"] = vertexAttributesJsonArray;
                submeshJson["IndexBuffer"] = indexBufferJson;
                size_t index = getReferenceIndex(submesh.material->getAssetPath(), reference);
                submeshJson["Material"] = "#" + std::to_string(index);
                submeshesJsonArray.emplace_back(submeshJson);
            }
            json["Submeshes"] = submeshesJsonArray;
        }

        virtual void deserialize(const Json& json, const std::vector<char>& binary, const std::vector<std::string>& reference) override
        {
            const Json& submeshesJsonArray = json["Submeshes"];
            for (const Json& submeshJson : submeshesJsonArray)
            {
                Submesh submesh;
                const Json& vertexAttributesJsonArray = submeshJson["VertexAttributes"];
                for (const Json& vertexAttributeJson : vertexAttributesJsonArray)
                {
                    uint32_t vertexAttributeIndex = vertexAttributeJson["Index"].get<uint32_t>();
                    osg::Array::Type vertexAttributeType = _sArrayTypeStringMap.backwardAt(vertexAttributeJson["Type"].get<std::string>());
                    size_t bufferOffset = vertexAttributeJson["Offset"].get<size_t>();
                    size_t bufferSize = vertexAttributeJson["Size"].get<size_t>();
                    osg::ref_ptr<osg::Array> array = createArrayByType(vertexAttributeType);
                    array->resizeArray(bufferSize / array->getElementSize());
                    std::memcpy(
                        const_cast<void*>(array->getDataPointer()),
                        &binary[bufferOffset],
                        bufferSize
                    );
                    submesh.vertexAttributes.insert(std::make_pair(vertexAttributeIndex, array));
                }

                const Json& indexBufferJson = submeshJson["IndexBuffer"];
                osg::PrimitiveSet::Type indexType = _sIndexTypeStringMap.backwardAt(indexBufferJson["Type"].get<std::string>());
                size_t bufferOffset = indexBufferJson["Offset"].get<size_t>();
                size_t bufferSize = indexBufferJson["Size"].get<size_t>();

                if (indexType == osg::PrimitiveSet::Type::DrawElementsUBytePrimitiveType)
                    submesh.indexBuffer = new osg::DrawElementsUByte(GL_TRIANGLES, bufferSize / sizeof(uint8_t));
                else if (indexType == osg::PrimitiveSet::Type::DrawElementsUShortPrimitiveType)
                    submesh.indexBuffer = new osg::DrawElementsUShort(GL_TRIANGLES, bufferSize / sizeof(uint16_t));
                else if (indexType == osg::PrimitiveSet::Type::DrawElementsUIntPrimitiveType)
                    submesh.indexBuffer = new osg::DrawElementsUInt(GL_TRIANGLES, bufferSize / sizeof(uint32_t));

                std::memcpy(
                    const_cast<void*>(submesh.indexBuffer->getDataPointer()),
                    &binary[bufferOffset],
                    bufferSize
                );

                const std::string& path = submeshJson["Material"].get<std::string>();
                int refIndex = std::stoi(path.substr(1));
                submesh.material = AssetManager::loadAsset<IMaterial>(reference[refIndex]);

                _submeshes.emplace_back(submesh);
            }
        }

        void setMaterial(size_t index, AMaterial* material)
        {
            if (index >= _submeshes.size())
                return;
            _submeshes[index].material = material;
        }

    private:
        struct Submesh
        {
            std::map<uint32_t, osg::ref_ptr<osg::Array>> vertexAttributes;
            osg::ref_ptr<osg::DrawElements> indexBuffer;
            osg::ref_ptr<AMaterial> material;
        };
        std::vector<Submesh> _submeshes;

        static const ConstBiMap<osg::Array::Type, std::string> _sArrayTypeStringMap;
        static const ConstBiMap<osg::PrimitiveSet::Type, std::string> _sIndexTypeStringMap;
        static osg::ref_ptr<osg::Array> createArrayByType(osg::Array::Type type);
    };
}
