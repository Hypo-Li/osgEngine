#pragma once
#include <ThirdParty/tinygltf/tiny_gltf.h>
#include <Core/Asset/MeshAsset.h>
#include <filesystem>

namespace xxx
{
    class GLTFLoader
    {
    public:
        static std::vector<osg::ref_ptr<MeshAsset>> load(const std::string& filePath)
        {
            std::filesystem::path path(filePath);
            tinygltf::Model gltfModel;
            tinygltf::TinyGLTF loader;
            std::string err;
            std::string warn;
            bool ret = false;
            if (path.extension().string() == ".glb")
                ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, filePath.c_str());
            else
                ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, filePath.c_str());

            if (!warn.empty())
                printf("Warn: %s\n", warn.c_str());
            if (!err.empty())
                printf("Err: %s\n", err.c_str());
            assert(ret);

            std::vector<osg::ref_ptr<MeshAsset>> meshes;

            for (tinygltf::Mesh& gltfMesh : gltfModel.meshes)
                meshes.push_back(createMesh(gltfMesh, gltfModel));

            //_sMapCache.clear();
            return meshes;
        }

    private:
        /*static struct
        {
            std::map<int, osg::ref_ptr<TextureAsset>> textureMap;
            std::map<int, osg::ref_ptr<MaterialAsset>> materialMap;
            std::map<int, osg::ref_ptr<MeshAsset>> meshMap;

            void clear()
            {
                textureMap.clear();
                materialMap.clear();
                meshMap.clear();
            }
        }_sMapCache;;*/

        static void copyBufferData(void* dst, const void* src, size_t size, size_t stride, size_t count)
        {
            if (stride > 0 && count > 0)
            {
                size_t elemSize = size / count;
                for (size_t i = 0; i < count; ++i)
                    memcpy((char*)dst + i * elemSize, (const char*)src + i * stride, elemSize);
            }
            else
                memcpy(dst, src, size);
        }

        static osg::ref_ptr<MeshAsset> createMesh(tinygltf::Mesh& gltfMesh, tinygltf::Model& gltfModel)
        {
            osg::ref_ptr<MeshAsset> meshAsset = new StaticMeshAsset;
            for (tinygltf::Primitive& gltfPrimitive : gltfMesh.primitives)
                meshAsset->addSubmesh(createSubmesh(gltfPrimitive, gltfModel));
            return meshAsset;
        }

        static MeshAsset::Submesh createSubmesh(tinygltf::Primitive& gltfPrimitive, tinygltf::Model& gltfModel)
        {
            MeshAsset::Submesh submesh;
            if (gltfPrimitive.mode != TINYGLTF_MODE_TRIANGLES)
            {
                std::cerr << "Unsupport mode!" << std::endl;
                return submesh;
            }

            tinygltf::Accessor indexAccessor = gltfModel.accessors[gltfPrimitive.indices];
            const tinygltf::BufferView& indexView = gltfModel.bufferViews[indexAccessor.bufferView];
            const tinygltf::Buffer& indexBuffer = gltfModel.buffers[indexView.buffer];

            if (indexView.target != TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER)
            {
                std::cerr << "Unsupport target!" << std::endl;
                return submesh;
            }

            int compSize = tinygltf::GetComponentSizeInBytes(indexAccessor.componentType);
            int count = indexAccessor.count;
            size_t stride = (indexView.byteStride > 0 && indexView.byteStride != compSize) ? indexView.byteStride : 0;
            size_t offset = indexView.byteOffset + indexAccessor.byteOffset;

            osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_TRIANGLES, count);
            if (compSize == 1)
            {
                for (uint32_t i = 0; i < count; ++i)
                    (*de)[i] = ((uint8_t*)&indexBuffer.data[offset])[i];
            }
            else if (compSize == 2)
            {
                for (uint32_t i = 0; i < count; ++i)
                    (*de)[i] = ((uint16_t*)&indexBuffer.data[offset])[i];
            }
            else if (compSize == 4)
            {
                memcpy(&(*de)[0], &indexBuffer.data[offset], count * compSize);
            }
            else
            {
                std::cerr << "Unsupport index type!" << std::endl;
                return submesh;
            }
            
            submesh._drawElements = de;

            for (auto itr = gltfPrimitive.attributes.begin(); itr != gltfPrimitive.attributes.end(); ++itr)
            {
                tinygltf::Accessor& attrAccessor = gltfModel.accessors[itr->second];
                const tinygltf::BufferView& attrView = gltfModel.bufferViews[attrAccessor.bufferView];
                if (!attrAccessor.count || attrView.buffer < 0) continue;

                size_t count = attrAccessor.count;
                const tinygltf::Buffer& buffer = gltfModel.buffers[attrView.buffer];
                int compNum = attrAccessor.type != TINYGLTF_TYPE_SCALAR ? attrAccessor.type : 1;
                int compSize = tinygltf::GetComponentSizeInBytes(attrAccessor.componentType);
                int copySize = count * (compSize * compNum);

                size_t offset = attrView.byteOffset + attrAccessor.byteOffset;
                size_t stride = (attrView.byteStride > 0 && attrView.byteStride != (compSize * compNum))
                    ? attrView.byteStride : 0;

                if (itr->first.compare("POSITION") == 0 && compSize == 4 && compNum == 3) // float and vec3
                {
                    osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array(count);
                    copyBufferData(&(*va)[0], &buffer.data[offset], copySize, stride, count);
                    submesh._vertexAttributes[0] = va;
                }
                else if (itr->first.compare("NORMAL") == 0 && compSize == 4 && compNum == 3)
                {
                    osg::ref_ptr<osg::Vec3Array> na = new osg::Vec3Array(count);
                    copyBufferData(&(*na)[0], &buffer.data[offset], copySize, stride, count);
                    submesh._vertexAttributes[1] = na;
                }
                else if (itr->first.compare("TANGENT") == 0 && compSize == 4 && compNum == 4)
                {
                    osg::ref_ptr<osg::Vec4Array> ta = new osg::Vec4Array(count);
                    copyBufferData(&(*ta)[0], &buffer.data[offset], copySize, stride, count);
                    submesh._vertexAttributes[6] = ta;
                }
                else if (itr->first.compare("COLOR_0") != std::string::npos && compSize == 4 && compNum == 4)
                {
                    osg::ref_ptr<osg::Vec4Array> ca = new osg::Vec4Array(count);
                    copyBufferData(&(*ca)[0], &buffer.data[offset], copySize, stride, count);
                    submesh._vertexAttributes[2] = ca;
                }
                else if (itr->first.find("TEXCOORD_") != std::string::npos && compSize == 4 && compNum == 2)
                {
                    osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array(count);
                    copyBufferData(&(*ta)[0], &buffer.data[offset], copySize, stride, count);
                    submesh._vertexAttributes[3 + atoi(itr->first.substr(9).c_str())] = ta;
                }
            }

            //auto& findResult = _sMapCache.materialMap.find(gltfPrimitive.material);
            //if (findResult == _sMapCache.materialMap.end())
            //{
            //    submesh._previewMaterial = createMaterial(gltfModel.materials[gltfPrimitive.material], gltfModel);
            //    _sMapCache.materialMap[gltfPrimitive.material] = submesh._previewMaterial;
            //}
            //else
            //{
            //    submesh._previewMaterial = findResult->second;
            //}

            return submesh;
        }

        /*static osg::ref_ptr<MaterialAsset> createMaterial(tinygltf::Material& gltfMaterial, tinygltf::Model& gltfModel)
        {
            MaterialTemplateAsset* materialTemplate = new MaterialTemplateAsset;

            if (gltfMaterial.alphaMode == "BLEND")
                materialTemplate->setAlphaMode(MaterialTemplateAsset::AlphaMode::Alpha_Blend);
            else if (gltfMaterial.alphaMode == "MASK")
                materialTemplate->setAlphaMode(MaterialTemplateAsset::AlphaMode::Alpha_Mask);
            else
                materialTemplate->setAlphaMode(MaterialTemplateAsset::AlphaMode::Opaque);

            materialTemplate->setDoubleSided(gltfMaterial.doubleSided);


        }*/
    };
}
