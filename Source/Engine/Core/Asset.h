#pragma once
#include "Object.h"
#include "Serialization/AssetLoader.h"
#include "Serialization/AssetSaver.h"

#include <osg/Referenced>
#include <osg/ref_ptr>
#include <string>
#include <fstream>
#include <filesystem>

namespace xxx
{
    class Asset : public osg::Referenced
    {
    public:
        Asset() = default;
        virtual ~Asset() = default;

        template <typename T = Object, std::enable_if_t<std::is_base_of_v<Object, T>, int> = 0>
        T* getPrimaryObject()
        {
            return dynamic_cast<T*>(mPrimaryObject.get());
        }

        template <typename T = Object, std::enable_if_t<std::is_base_of_v<Object, T>, int> = 0>
        T* findObjectByGuid(Guid guid)
        {
            auto findResult = mSecondaryObjects.find(guid);
            if (findResult != mSecondaryObjects.end())
                return dynamic_cast<T*>(findResult->second.get());
            return nullptr;
        }

        void load()
        {
            if (!mIsLoaded)
                forceLoad();
        }

        void forceLoad()
        {
            std::filesystem::path assetPath;
            std::ifstream ifs(assetPath, std::ios::binary | std::ios::ate);
            AssetSerializer* assetLoader = new AssetLoader(this);
            // process asset header, and fill asset loader's buffer
            size_t fileSize = ifs.tellg();
            ifs.seekg(std::ios::beg);
        }

    private:
        std::string mPath;
        bool mIsLoaded = false;
        bool mNeedSave = false;
        osg::ref_ptr<Object> mPrimaryObject;
        std::unordered_map<Guid, osg::ref_ptr<Object>> mSecondaryObjects;
    };
}
