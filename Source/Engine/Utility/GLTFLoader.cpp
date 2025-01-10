#include "GLTFLoader.h"

namespace xxx
{
    osg::Node* GLTFLoader::load(const std::string& filePath)
    {
        std::filesystem::path path(filePath);
        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err;
        std::string warn;
        bool ret = false;
        if (path.extension().string() == ".glb")
            ret = loader.LoadBinaryFromFile(&model, &err, &warn, filePath.c_str());
        else
            ret = loader.LoadASCIIFromFile(&model, &err, &warn, filePath.c_str());

        if (!warn.empty())
            printf("Warn: %s\n", warn.c_str());
        if (!err.empty())
            printf("Err: %s\n", err.c_str());
        assert(ret);
        return createModel(model);
    }

    const std::map<std::string, GLTFLoader::GLTFExtension> GLTFLoader::sSupportedExtensionMap = {
        {"KHR_materials_emissive_strength", GLTFExtension::KHR_materials_emissive_strength},
    };

    void GLTFLoader::copyBufferData(void* dst, const void* src, size_t size, size_t stride, size_t count)
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

    osg::Node* GLTFLoader::createModel(tinygltf::Model& model)
    {
        if (model.scenes.size() == 0)
            return nullptr;
        osg::ref_ptr<osg::Group> root = new osg::Group;
        MapCache cache;
        for (int nodeIndex : model.scenes[model.defaultScene].nodes)
        {
            osg::Node* node = createNode(nodeIndex, model, cache);
            if (node)
                root->addChild(node);
        }
        return root.release();
    }

    osg::Node* GLTFLoader::createNode(int index, tinygltf::Model& model, MapCache& cache)
    {
        auto found = cache.nodeMap.find(index);
        if (found != cache.nodeMap.end())
            return found->second.get();

        tinygltf::Node& node = model.nodes[index];
        osg::ref_ptr<osg::Group> group;

        if (node.translation.empty() && node.rotation.empty() && node.scale.empty() && node.matrix.empty())
        {
            group = new osg::Group;
        }
        else
        {
            osg::ref_ptr<osg::MatrixTransform> matrixTransform = new osg::MatrixTransform;
            osg::Matrixd matrix;
            if (!node.matrix.empty())
                matrix *= osg::Matrixd(&node.matrix[0]);
            if (!node.scale.empty())
                matrix *= osg::Matrixd::scale(node.scale[0], node.scale[1], node.scale[2]);
            if (!node.rotation.empty())
                matrix *= osg::Matrixd::rotate(osg::Quat(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]));
            if (!node.translation.empty())
                matrix *= osg::Matrixd::translate(node.translation[0], node.translation[1], node.translation[2]);
            matrixTransform->setMatrix(matrix);
            group = matrixTransform;
        }

        group->setName(node.name);
        if (node.mesh >= 0)
        {
            group->addChild(createMesh(node.mesh, model, cache));
        }

        for (int child : node.children)
        {
            group->addChild(createNode(child, model, cache));
        }

        cache.nodeMap.insert(std::make_pair(index, group));
        return group.release();
    }

    osg::Node* GLTFLoader::createMesh(int index, tinygltf::Model& model, MapCache& cache)
    {
        auto found = cache.meshMap.find(index);
        if (found != cache.meshMap.end())
            return found->second.get();

        tinygltf::Mesh& mesh = model.meshes[index];
        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->setName(mesh.name);
        for (tinygltf::Primitive& primitive : mesh.primitives)
        {
            osg::Geometry* geometry = createPrimitive(primitive, model);
            if (geometry)
            {
                if (primitive.material >= 0)
                    geometry->setStateSet(createMaterial(primitive.material, model, cache));
                geode->addDrawable(geometry);
            }
        }

        cache.meshMap.insert(std::make_pair(index, geode));
        return geode.release();
    }

    osg::StateSet* GLTFLoader::createMaterial(int index, tinygltf::Model& model, MapCache& cache)
    {
        auto found = cache.materialMap.find(index);
        if (found != cache.materialMap.end())
            return found->second.get();

        tinygltf::Material& material = model.materials[index];
        osg::ref_ptr<osg::StateSet> stateSet = new osg::StateSet;

        if (material.alphaMode == "BLEND")
        {
            stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
            stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
            stateSet->setAttributeAndModes(new osg::Depth(osg::Depth::LESS, 0.0, 1.0, false));
            stateSet->addUniform(new osg::Uniform("uAlphaMode", 2u));
        }
        else if (material.alphaMode == "MASK")
        {
            stateSet->setRenderingHint(osg::StateSet::OPAQUE_BIN);
            stateSet->addUniform(new osg::Uniform("uAlphaMode", 1u));
        }
        else
        {
            stateSet->setRenderingHint(osg::StateSet::OPAQUE_BIN);
            stateSet->addUniform(new osg::Uniform("uAlphaMode", 0u));
        }

        if (!material.doubleSided)
        {
            stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
            stateSet->setAttributeAndModes(new osg::CullFace(osg::CullFace::BACK));
        }

        osg::ref_ptr<osg::Texture2D> baseColorTexture;
        int baseColorTextureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
        if (baseColorTextureIndex >= 0)
            baseColorTexture = createTexture(baseColorTextureIndex, model, cache);
        else
            baseColorTexture = createDefaultWhiteTexture();
        stateSet->addUniform(new osg::Uniform("uBaseColorTexture", 0));
        stateSet->setTextureAttributeAndModes(0, baseColorTexture);
        std::vector<double>& baseColorFactor = material.pbrMetallicRoughness.baseColorFactor;
        osg::ref_ptr<osg::Uniform> baseColorFactorUniform = new osg::Uniform("uBaseColorFactor", osg::Vec4(baseColorFactor.at(0), baseColorFactor.at(1), baseColorFactor.at(2), baseColorFactor.at(3)));
        stateSet->addUniform(baseColorFactorUniform);

        osg::ref_ptr<osg::Texture2D> metallicRoughnessTexture;
        int metallicRoughnessIndex = material.pbrMetallicRoughness.metallicRoughnessTexture.index;
        if (metallicRoughnessIndex >= 0)
            metallicRoughnessTexture = createTexture(metallicRoughnessIndex, model, cache);
        else
            metallicRoughnessTexture = createDefaultWhiteTexture();
        stateSet->addUniform(new osg::Uniform("uMetallicRoughnessTexture", 1));
        stateSet->setTextureAttributeAndModes(1, metallicRoughnessTexture);
        double metallicFactor = material.pbrMetallicRoughness.metallicFactor;
        double roughnessFactor = material.pbrMetallicRoughness.roughnessFactor;
        osg::ref_ptr<osg::Uniform> metallicRoughnessFactorUniform = new osg::Uniform("uMetallicRoughnessFactor", osg::Vec2(metallicFactor, roughnessFactor));
        stateSet->addUniform(metallicRoughnessFactorUniform);

        osg::ref_ptr<osg::Texture2D> normalTexture;
        int normalTextureIndex = material.normalTexture.index;
        if (normalTextureIndex >= 0)
            normalTexture = createTexture(normalTextureIndex, model, cache);
        else
            normalTexture = createDefaultNormalTexture();
        stateSet->addUniform(new osg::Uniform("uNormalTexture", 2));
        stateSet->setTextureAttributeAndModes(2, normalTexture);

        osg::ref_ptr<osg::Texture2D> emissiveTexture;
        int emissiveTextureIndex = material.emissiveTexture.index;
        if (emissiveTextureIndex >= 0)
            emissiveTexture = createTexture(emissiveTextureIndex, model, cache);
        else
            emissiveTexture = createDefaultWhiteTexture();
        stateSet->addUniform(new osg::Uniform("uEmissiveTexture", 3));
        stateSet->setTextureAttributeAndModes(3, emissiveTexture);
        std::vector<double>& emissiveFactor = material.emissiveFactor;
        osg::ref_ptr<osg::Uniform> emissiveFactorUniform = new osg::Uniform("uEmissiveFactor", osg::Vec3(emissiveFactor.at(0), emissiveFactor.at(1), emissiveFactor.at(2)));
        stateSet->addUniform(emissiveFactorUniform);

        osg::ref_ptr<osg::Texture2D> occlusionTexture;
        int occlusionTextureIndex = material.occlusionTexture.index;
        if (occlusionTextureIndex >= 0)
            occlusionTexture = createTexture(occlusionTextureIndex, model, cache);
        else
            occlusionTexture = createDefaultWhiteTexture();
        stateSet->addUniform(new osg::Uniform("uOcclusionTexture", 4));
        stateSet->setTextureAttributeAndModes(4, occlusionTexture);

        // extensions
        for (auto& extension : material.extensions)
        {
            auto found = sSupportedExtensionMap.find(extension.first);
            if (found != sSupportedExtensionMap.end())
            {
                switch (found->second)
                {
                case GLTFExtension::KHR_materials_emissive_strength:
                {
                    double emissiveStrength = extension.second.Get("emissiveStrength").GetNumberAsDouble();
                    emissiveFactorUniform->set(osg::Vec3(emissiveFactor.at(0) * emissiveStrength, emissiveFactor.at(1) * emissiveStrength, emissiveFactor.at(2) * emissiveStrength));
                    break;
                }
                default:
                    break;
                }
            }
        }

        cache.materialMap.insert(std::make_pair(index, stateSet));
        return stateSet.release();
    }

    osg::Texture2D* GLTFLoader::createTexture(int index, tinygltf::Model& model, MapCache& cache)
    {
        auto found = cache.textureMap.find(index);
        if (found != cache.textureMap.end())
            return found->second.get();

        tinygltf::Texture& texture = model.textures[index];
        osg::ref_ptr<osg::Texture2D> osgTexture2D = new osg::Texture2D;
        tinygltf::Sampler& sampler = model.samplers[texture.sampler];
        osg::ref_ptr<osg::Image> osgImage = createImage(texture.source, model, cache);
        osgTexture2D->setResizeNonPowerOfTwoHint(false);
        osgTexture2D->setWrap(osg::Texture::WRAP_S, static_cast<osg::Texture::WrapMode>(sampler.wrapS));
        osgTexture2D->setWrap(osg::Texture::WRAP_T, static_cast<osg::Texture::WrapMode>(sampler.wrapT));
        osgTexture2D->setFilter(osg::Texture::MIN_FILTER, static_cast<osg::Texture::FilterMode>(sampler.minFilter));
        osgTexture2D->setFilter(osg::Texture::MAG_FILTER, static_cast<osg::Texture::FilterMode>(sampler.magFilter));
        osgTexture2D->setImage(osgImage);

        cache.textureMap.insert(std::make_pair(index, osgTexture2D));
        return osgTexture2D.release();
    }

    osg::Texture2D* GLTFLoader::createDefaultTexture(osg::Vec4 color)
    {
        osg::ref_ptr<osg::Image> image = new osg::Image;
        image->allocateImage(1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE);
        image->setInternalTextureFormat(GL_RGBA);

        osg::Vec4ub* ptr = (osg::Vec4ub*)image->data();
        *ptr = osg::Vec4ub(color[0] * 255, color[1] * 255, color[2] * 255, color[3] * 255);

        osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
        texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
        texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
        texture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        texture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        texture->setImage(image.get());
        return texture.release();
    }

    osg::Texture2D* GLTFLoader::createDefaultWhiteTexture()
    {
        static osg::ref_ptr<osg::Texture2D> whiteTexture;
        if (whiteTexture)
            return whiteTexture;
        whiteTexture = createDefaultTexture(osg::Vec4(1.0, 1.0, 1.0, 1.0));
        return whiteTexture;
    }

    osg::Texture2D* GLTFLoader::createDefaultNormalTexture()
    {
        static osg::ref_ptr<osg::Texture2D> normalTexture;
        if (normalTexture)
            return normalTexture;
        normalTexture = createDefaultTexture(osg::Vec4(0.5, 0.5, 1.0, 1.0));
        return normalTexture;
    }

    osg::Image* GLTFLoader::createImage(int index, tinygltf::Model& model, MapCache& cache)
    {
        auto found = cache.imageMap.find(index);
        if (found != cache.imageMap.end())
            return found->second.get();

        tinygltf::Image& image = model.images[index];
        osg::ref_ptr<osg::Image> osgImage = new osg::Image;
        GLenum format = GL_RGBA;
        if (image.component == 1) format = GL_RED;
        else if (image.component == 2) format = GL_RG;
        else if (image.component == 3) format = GL_RGB;
        GLenum type = GL_UNSIGNED_BYTE;
        if (image.bits == 16) type = GL_UNSIGNED_SHORT;
        osgImage->setFileName(image.uri);
        osgImage->allocateImage(image.width, image.height, 1, format, type);
        switch (image.component)
        {
        case 1: osgImage->setInternalTextureFormat(GL_R8); break;
        case 2: osgImage->setInternalTextureFormat(GL_RG8); break;
        case 3: osgImage->setInternalTextureFormat(GL_RGB8); break;
        default: osgImage->setInternalTextureFormat(GL_RGBA8); break;
        }
        memcpy(osgImage->data(), &image.image[0], osgImage->getTotalSizeInBytes());

        cache.imageMap.insert(std::make_pair(index, osgImage));
        return osgImage.release();
    }

    osg::Geometry* GLTFLoader::createPrimitive(tinygltf::Primitive& primitive, tinygltf::Model& model)
    {
        osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
        geometry->setUseDisplayList(false);
        geometry->setUseVertexBufferObjects(true);

        for (auto itr = primitive.attributes.begin(); itr != primitive.attributes.end(); ++itr)
        {
            tinygltf::Accessor& attrAccessor = model.accessors[itr->second];
            const tinygltf::BufferView& attrView = model.bufferViews[attrAccessor.bufferView];
            if (!attrAccessor.count || attrView.buffer < 0) continue;

            size_t count = attrAccessor.count;
            const tinygltf::Buffer& buffer = model.buffers[attrView.buffer];
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
                va->setNormalize(attrAccessor.normalized);
                geometry->setVertexArray(va);
            }
            else if (itr->first.compare("NORMAL") == 0 && compSize == 4 && compNum == 3)
            {
                osg::ref_ptr<osg::Vec3Array> na = new osg::Vec3Array(count);
                copyBufferData(&(*na)[0], &buffer.data[offset], copySize, stride, count);
                na->setNormalize(attrAccessor.normalized);
                //geometry->setVertexAttribArray(1, na, osg::Array::BIND_PER_VERTEX);
                geometry->setNormalArray(na, osg::Array::BIND_PER_VERTEX);
            }
            else if (itr->first.compare("TANGENT") == 0 && compSize == 4 && compNum == 4)
            {
                osg::ref_ptr<osg::Vec4Array> ta = new osg::Vec4Array(count);
                copyBufferData(&(*ta)[0], &buffer.data[offset], copySize, stride, count);
                ta->setNormalize(attrAccessor.normalized);
                geometry->setVertexAttribArray(6, ta, osg::Array::BIND_PER_VERTEX);
            }
            else if (itr->first.find("TEXCOORD_") != std::string::npos && compSize == 4 && compNum == 2)
            {
                osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array(count);
                copyBufferData(&(*ta)[0], &buffer.data[offset], copySize, stride, count);
                ta->setNormalize(attrAccessor.normalized);
                //geometry->setVertexAttribArray(texcoordIndex++, ta, osg::Array::BIND_PER_VERTEX);
                geometry->setTexCoordArray(atoi(itr->first.substr(9).c_str()), ta, osg::Array::BIND_PER_VERTEX);
            }
        }

        tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];
        const tinygltf::BufferView& indexView = model.bufferViews[indexAccessor.bufferView];
        osg::Vec3Array* va = static_cast<osg::Vec3Array*>(geometry->getVertexArray());
        if (!va || (va && va->empty())) return nullptr;

        osg::ref_ptr<osg::PrimitiveSet> p;
        if (indexView.target == 0)
            p = new osg::DrawArrays(GL_POINTS, 0, va->size());
        else
        {
            const tinygltf::Buffer& indexBuffer = model.buffers[indexView.buffer];
            int compSize = tinygltf::GetComponentSizeInBytes(indexAccessor.componentType);
            int count = indexAccessor.count;
            size_t stride = (indexView.byteStride > 0 && indexView.byteStride != compSize)
                ? indexView.byteStride : 0;
            size_t offset = indexView.byteOffset + indexAccessor.byteOffset;
            switch (compSize)
            {
            case 1:
            {
                osg::DrawElementsUByte* de = new osg::DrawElementsUByte(GL_POINTS, count);
                copyBufferData(&(*de)[0], &indexBuffer.data[offset], count * compSize, stride, count);
                p = de;
            }
            break;
            case 2:
            {
                osg::DrawElementsUShort* de = new osg::DrawElementsUShort(GL_POINTS, count);
                copyBufferData(&(*de)[0], &indexBuffer.data[offset], count * compSize, stride, count);
                p = de;
            }
            break;
            case 4:
            {
                osg::DrawElementsUInt* de = new osg::DrawElementsUInt(GL_POINTS, count);
                copyBufferData(&(*de)[0], &indexBuffer.data[offset], count * compSize, stride, count);
                p = de;
            }
            break;
            }
        }

        switch (primitive.mode)
        {
        case TINYGLTF_MODE_POINTS: p->setMode(GL_POINTS); break;
        case TINYGLTF_MODE_LINE: p->setMode(GL_LINES); break;
        case TINYGLTF_MODE_LINE_LOOP: p->setMode(GL_LINE_LOOP); break;
        case TINYGLTF_MODE_LINE_STRIP: p->setMode(GL_LINE_STRIP); break;
        case TINYGLTF_MODE_TRIANGLES: p->setMode(GL_TRIANGLES); break;
        case TINYGLTF_MODE_TRIANGLE_STRIP: p->setMode(GL_TRIANGLE_STRIP); break;
        case TINYGLTF_MODE_TRIANGLE_FAN: p->setMode(GL_TRIANGLE_FAN); break;
        }
        geometry->addPrimitiveSet(p.get());

        if (!geometry->getVertexAttribArray(6))
        {
            osg::ref_ptr<osgUtil::TangentSpaceGenerator> tangentSpaceGenerator = new osgUtil::TangentSpaceGenerator;
            tangentSpaceGenerator->generate(geometry, 0);
            geometry->setVertexAttribArray(6, tangentSpaceGenerator->getTangentArray(), osg::Array::BIND_PER_VERTEX);
        }

        return geometry.release();
    }
}
