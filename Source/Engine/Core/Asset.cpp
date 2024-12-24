#include "Asset.h"
#include <Engine/Core/Context.h>

namespace xxx
{
    using namespace refl;

    Asset::Asset(const std::string& path, Object* rootObject)
    {
        setPath(path);
        setRootObject(rootObject);
        if (mState == State::Unloaded)
            initializeClassAndRefAssetPaths();
    }

    void Asset::load()
    {
        std::ifstream ifs(convertAssetPathToPhysicalPath(mPath), std::ios::binary);
        AssetLoader loader(this);
        AssetHeader header;

        readAssetHeader(ifs, header);
        readStringTable(ifs, header, loader);
        readImportTable(ifs, header, loader);
        readObjectBuffers(ifs, header, loader);

        Object* rootObject = nullptr;
        loader.serializeClass(nullptr, &rootObject);

        mRootObject = rootObject;
        mState = State::Loaded;
    }

    void Asset::unload()
    {
        if (mState == State::Changed)
        {
            //LOG_WARN("Asset was changed, need to save first.");
        }
        else if (mState == State::Loaded && mRootObject->referenceCount() == 1)
        {
            mRootObject = nullptr;
            mState = State::Unloaded;
        }
    }

    void Asset::save()
    {
        Object* rootObject = mRootObject;
        AssetSaver saver(this);

        saver.serializeClass(nullptr, &rootObject);

        std::ofstream ofs(convertAssetPathToPhysicalPath(mPath), std::ios::binary);
        AssetHeader header;
        writeAssetHeader(ofs, header, saver);
        writeStringTable(ofs, header, saver);
        writeImportTable(ofs, header, saver);
        writeObjectBuffers(ofs, header, saver);

        if (mState == State::Changed)
            mState = State::Loaded;
    }

    std::string Asset::convertPhysicalPathToAssetPath(const std::filesystem::path& fullPath)
    {
        if (fullPath.extension() != sAssetExtension)
            return std::string();

        const std::filesystem::path& engineAssetPath = Context::get().getEngineAssetPath();
        if (fullPath.compare(engineAssetPath) > 0)
        {
            std::filesystem::path relativePath = fullPath.lexically_relative(engineAssetPath);
            relativePath.replace_extension("");
            std::string assetPath = "Engine/" + relativePath.string();
            std::replace(assetPath.begin(), assetPath.end(), '\\', '/');
            return assetPath;
        }

        const std::filesystem::path& projectAssetPath = Context::get().getProjectAssetPath();
        if (fullPath.compare(projectAssetPath) > 0)
        {
            std::filesystem::path relativePath = fullPath.lexically_relative(projectAssetPath);
            relativePath.replace_extension("");
            std::string assetPath = "Project/" + relativePath.string();
            std::replace(assetPath.begin(), assetPath.end(), '\\', '/');
            return assetPath;
        }

        return std::string();
    }

    std::filesystem::path Asset::convertAssetPathToPhysicalPath(const std::string& assetPath)
    {
        std::filesystem::path fullPath;
        if (assetPath.substr(0, 7) == "Engine/")
        {
            fullPath = Context::get().getEngineAssetPath();
            fullPath /= assetPath.substr(7);
            fullPath += sAssetExtension;
            return fullPath;
        }

        if (assetPath.substr(0, 8) == "Project/")
        {
            fullPath = Context::get().getProjectAssetPath();
            fullPath /= assetPath.substr(8);
            fullPath += sAssetExtension;
            return fullPath;
        }

        // LogError illegal path
        return std::filesystem::path();
    }

    void Asset::setPath(const std::string& path)
    {
        mPath = path;
        mName = mPath.substr(mPath.find_last_of('/') + 1);
    }

    void Asset::setRootObject(Object* rootObject)
    {
        if (mRootObject == rootObject)
            return;

        mRootObject = rootObject;
        if (rootObject)
        {
            mGuid = rootObject->getGuid();
            mClass = rootObject->getClass();
        }
        else
        {
            mGuid = Guid();
            mClass = nullptr;
        }

        mState = mRootObject ? State::Changed : State::Unloaded;
    }

    void Asset::initializeClassAndRefAssetPaths()
    {
        std::filesystem::path physicalPath = convertAssetPathToPhysicalPath(mPath);
        std::ifstream ifs(physicalPath, std::ios::binary);
        if (!ifs.is_open())
        {
            //LOG_ERROR("Cannot open asset file: {}", mPath);
            if (mRootObject)
            {
                mGuid = mRootObject->getGuid();
                mClass = mRootObject->getClass();
            }
        }
        else
        {
            AssetLoader loader(this);
            AssetHeader header;
            // read Asset Header
            readAssetHeader(ifs, header);
            mGuid = header.guid;

            // read String Table
            readStringTable(ifs, header, loader);
            mClass = Reflection::getClass(loader.getString(0));

            // read Import Table
            readImportTable(ifs, header, loader);
            uint32_t importTableSize = loader.getImportTableSize();
            for (uint32_t i = 0; i < importTableSize; ++i)
                mRefAssetPaths.insert(loader.getString(loader.getImportItem(i)));
        }
    }

    void Asset::readAssetHeader(std::ifstream& ifs, AssetHeader& header)
    {
        ifs.seekg(std::ios::beg);
        ifs.read((char*)(&header), sizeof(AssetHeader));
    }

    void Asset::readStringTable(std::ifstream& ifs, const AssetHeader& header, AssetSerializer& serializer)
    {
        std::vector<uint8_t> buffer(header.stringTableSize);
        uint8_t* data = buffer.data();

        ifs.seekg(sizeof(AssetHeader));
        ifs.read((char*)(data), header.stringTableSize);

        uint32_t stringCount = *(uint32_t*)(data);
        data += sizeof(uint32_t);

        for (uint32_t i = 0; i < stringCount; ++i)
        {
            uint32_t stringLength = *(uint32_t*)(data);
            data += sizeof(uint32_t);
            serializer.addString(std::string((char*)data, stringLength));
            data += stringLength;
        }
    }

    void Asset::readImportTable(std::ifstream& ifs, const AssetHeader& header, AssetSerializer& serializer)
    {
        std::vector<uint8_t> buffer(header.importTableSize);
        uint8_t* data = buffer.data();

        ifs.seekg(sizeof(AssetHeader) + header.stringTableSize);
        ifs.read((char*)(data), header.importTableSize);

        uint32_t importCount = *(uint32_t*)(data);
        data += sizeof(uint32_t);

        for (uint32_t i = 0; i < importCount; ++i)
        {
            uint32_t importItem = *(uint32_t*)(data);
            data += sizeof(uint32_t);
            serializer.addImportItem(importItem);
        }
    }

    void Asset::readObjectBuffers(std::ifstream& ifs, const AssetHeader& header, AssetSerializer& serializer)
    {
        ifs.seekg(sizeof(AssetHeader) + header.stringTableSize + header.importTableSize);
        for (uint32_t i = 0; i < header.objectBufferCount; ++i)
        {
            size_t objectBufferSize = 0;
            ifs.read((char*)(&objectBufferSize), sizeof(size_t));
            serializer.pushObjectBuffer(serializer.createObjectBuffer(objectBufferSize));
            ifs.read((char*)(serializer.getCurrentObjectBuffer()->getData()), objectBufferSize);
            serializer.popObjectBuffer();
        }
    }

    void Asset::writeAssetHeader(std::ofstream& ofs, AssetHeader& header, AssetSerializer& serializer)
    {
        //header.magic = 0;
        header.flags = 0;
        header.guid = serializer.getAsset()->getGuid();
        header.stringTableSize = sizeof(uint32_t);
        uint32_t stringCount = serializer.getStringTableSize();
        for (uint32_t i = 0; i < stringCount; ++i)
            header.stringTableSize += sizeof(uint32_t) + serializer.getString(i).size();
        // 保证string table大小可整除4
        header.stringTableSize = (header.stringTableSize + 3) & ~3;
        header.importTableSize = sizeof(uint32_t) + serializer.getImportTableSize() * sizeof(uint32_t);
        header.objectBufferCount = serializer.getObjectBufferCount();
        ofs.write((const char*)(&header), sizeof(AssetHeader));
    }

    void Asset::writeStringTable(std::ofstream& ofs, const AssetHeader& header, AssetSerializer& serializer)
    {
        std::vector<uint8_t> buffer(header.stringTableSize);
        uint8_t* data = buffer.data();

        uint32_t stringCount = serializer.getStringTableSize();
        *(uint32_t*)(data) = stringCount;
        data += sizeof(uint32_t);

        for (uint32_t i = 0; i < stringCount; ++i)
        {
            const std::string& str = serializer.getString(i);
            *(uint32_t*)(data) = static_cast<uint32_t>(str.size());
            data += sizeof(uint32_t);
            std::memcpy(data, str.data(), str.size());
            data += str.size();
        }

        ofs.write((const char*)(buffer.data()), header.stringTableSize);
    }

    void Asset::writeImportTable(std::ofstream& ofs, const AssetHeader& header, AssetSerializer& serializer)
    {
        std::vector<uint8_t> buffer(header.importTableSize);
        uint8_t* data = buffer.data();

        uint32_t importItemCount = serializer.getImportTableSize();
        *(uint32_t*)(data) = importItemCount;
        data += sizeof(uint32_t);

        for (uint32_t i = 0; i < importItemCount; ++i)
        {
            *(uint32_t*)(data) = serializer.getImportItem(i);
            data += sizeof(uint32_t);
        }

        ofs.write((const char*)(buffer.data()), header.importTableSize);
    }

    void Asset::writeObjectBuffers(std::ofstream& ofs, const AssetHeader& header, AssetSerializer& serializer)
    {
        for (uint32_t i = 0; i < header.objectBufferCount; ++i)
        {
            serializer.pushObjectBuffer(i);
            AssetSerializer::ObjectBuffer* objectBuffer = serializer.getCurrentObjectBuffer();
            size_t bufferSize = objectBuffer->getSize();
            uint8_t* bufferData = objectBuffer->getData();
            ofs.write((const char*)(&bufferSize), sizeof(size_t));
            ofs.write((const char*)(bufferData), bufferSize);
            serializer.popObjectBuffer();
        }
    }
}
