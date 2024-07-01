#pragma once
#include "Asset.h"
#include "MaterialAsset.h"
#include <osg/Geometry>

namespace xxx
{
    class MeshRenderer;
    class StaticMeshAsset : public Asset
    {
        friend class MeshRenderer;
    public:
        StaticMeshAsset() : Asset(Type::StaticMesh) {}
        virtual ~StaticMeshAsset() = default;

        virtual void serialize(Json& json, std::vector<char>& binary, std::vector<std::string>& reference) const override
        {
            size_t binarySize = 0;
            for (const auto& submesh : _submeshes)
            {
                for (const auto& vertexAttribute : submesh._vertexAttributes)
                    binarySize += vertexAttribute.second->getDataSize();
                binarySize += submesh._drawElements->size();
            }
            binary.resize(binarySize);

            size_t offset = 0;
            std::vector<Json> submeshesJsonArray;
            for (const auto& submesh : _submeshes)
            {
                std::vector<Json> vertexAttributesJsonArray;
                for (const auto& vertexAttribute : submesh._vertexAttributes)
                {
                    Json vertexAttributeJson;
                    vertexAttributeJson["Index"] = vertexAttribute.first;
                    vertexAttributeJson["Type"] = _sArrayTypeStringMap.forwardAt(vertexAttribute.second->getType());
                    vertexAttributeJson["BufferOffset"] = offset;
                    vertexAttributeJson["BufferSize"] = vertexAttribute.second->getDataSize();
                    vertexAttributesJsonArray.emplace_back(vertexAttributeJson);
                    std::memcpy(
                        &binary[offset],
                        (char*)vertexAttribute.second->getDataPointer(),
                        vertexAttribute.second->getDataSize()
                    );
                    offset += vertexAttribute.second->getDataSize();
                }

                Json indicesJson;
                indicesJson["BufferOffset"] = offset;
                indicesJson["BufferSize"] = submesh._drawElements->size();
                std::memcpy(
                    &binary[offset],
                    (char*)submesh._drawElements->getDataPointer(),
                    submesh._drawElements->size()
                );
                offset += submesh._drawElements->size();

                Json submeshJson;
                submeshJson["Vertices"] = vertexAttributesJsonArray;
                submeshJson["Indices"] = indicesJson;
                size_t index = getReferenceIndex(submesh._previewMaterial->getPath(), reference);
                submeshJson["PreviewMaterial"] = "#" + std::to_string(index);
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
                const Json& vertexAttributesJsonArray = submeshJson["Vertices"];
                for (const Json& vertexAttributeJson : vertexAttributesJsonArray)
                {
                    uint32_t vertexAttributeIndex = vertexAttributeJson["Index"].get<uint32_t>();
                    osg::Array::Type vertexAttributeType = _sArrayTypeStringMap.backwardAt(vertexAttributeJson["Type"].get<std::string>());
                    size_t bufferOffset = vertexAttributeJson["BufferOffset"].get<size_t>();
                    size_t bufferSize = vertexAttributeJson["BufferSize"].get<size_t>();
                    osg::ref_ptr<osg::Array> array = createArrayByType(vertexAttributeType);
                    array->resizeArray(bufferSize / array->getElementSize());
                    std::memcpy(const_cast<void*>(array->getDataPointer()), &binary[bufferOffset], bufferSize);
                    submesh._vertexAttributes.emplace(vertexAttributeIndex, array);
                }

                const Json& indicesJson = submeshJson["Indices"];
                size_t bufferOffset = indicesJson["BufferOffset"].get<size_t>();
                size_t bufferSize = indicesJson["BufferSize"].get<size_t>();
                submesh._drawElements = new osg::DrawElementsUInt(GL_TRIANGLES, bufferSize / sizeof(uint32_t));
                std::memcpy(const_cast<void*>(submesh._drawElements->getDataPointer()), &binary[bufferOffset], bufferSize);

                const std::string& path = submeshJson["PreviewMaterial"].get<std::string>();
                int index = std::stoi(path.substr(1));
                submesh._previewMaterial = dynamic_cast<MaterialAsset*>(AssetManager::loadAsset(reference[index]));

                _submeshes.emplace_back(submesh);
            }
        }

        uint32_t getSubmeshCount()
        {
            return _submeshes.size();
        }

        void setPreviewMaterial(MaterialAsset* material, uint32_t index)
        {
            _submeshes[index]._previewMaterial = material;
            // refresh scene to update preview material
        }

    private:
        struct Submesh
        {
            std::map<uint32_t, osg::ref_ptr<osg::Array>> _vertexAttributes;
            osg::ref_ptr<osg::DrawElementsUInt> _drawElements;
            osg::ref_ptr<MaterialAsset> _previewMaterial;
        };
        std::vector<Submesh> _submeshes;

        static const ConstBiMap<osg::Array::Type, std::string> _sArrayTypeStringMap;
        static osg::ref_ptr<osg::Array> createArrayByType(osg::Array::Type type);
    };
}
