#pragma once
#include "Object.h"
#include "Serialization/AssetLoader.h"
#include "Serialization/AssetSaver.h"

#include <osg/Referenced>
#include <osg/ref_ptr>
#include <string>
#include <fstream>
#include <filesystem>
#include <unordered_set>

namespace xxx
{
    /**
    * Asset Header
    * String Table
    * Import Table
    * Object Buffers
    */

    struct AssetHeader
    {
        uint32_t magic = 0x54534158; // XXXASSET
        uint32_t flags = 0;
        Guid guid;
        uint32_t stringTableSize;
        uint32_t importTableSize;
        uint32_t objectBufferCount;
    };

    class AssetManager;

    class Asset : public osg::Referenced
    {
        friend class AssetManager;
    public:
        const std::string& getPath() const
        {
            return mPath;
        }

        const std::string& getName() const
        {
            return mName;
        }

        Guid getGuid() const
        {
            return mGuid;
        }

        enum class State
        {
            Unloaded,
            Loaded,
            Changed,
        };

        State getState() const
        {
            return mState;
        }

        inline refl::Class* getClass() const
        {
            return mClass;
        }

        template <typename T = Object> requires std::is_base_of_v<Object, T>
        T* getRootObject() const
        {
            return dynamic_cast<T*>(mRootObject.get());
        }

        template <typename T = Object> requires std::is_base_of_v<Object, T>
        T* getRootObjectSafety()
        {
            if (mState == State::Unloaded)
                load();
            return dynamic_cast<T*>(mRootObject.get());
        }

        void load();

        void unload();

        void save();

        static std::string convertPhysicalPathToAssetPath(const std::filesystem::path& fullPath);

        static std::filesystem::path convertAssetPathToPhysicalPath(const std::string& assetPath);

    protected:
        Asset(const std::string& path, Object* rootObject);

        void setPath(const std::string& path);

        void setRootObject(Object* rootObject);

        inline static const std::string_view sAssetExtension = ".xast";

    private:
        std::string mPath;
        std::string mName;
        refl::Class* mClass;
        Guid mGuid;
        State mState = State::Unloaded;
        osg::ref_ptr<Object> mRootObject;
        std::unordered_set<std::string> mRefAssetPaths;

        void initializeClassAndRefAssetPaths();

        static void readAssetHeader(std::ifstream& ifs, AssetHeader& header);

        static void readStringTable(std::ifstream& ifs, const AssetHeader& header, AssetSerializer& serializer);

        static void readImportTable(std::ifstream& ifs, const AssetHeader& header, AssetSerializer& serializer);

        static void readObjectBuffers(std::ifstream& ifs, const AssetHeader& header, AssetSerializer& serializer);

        static void writeAssetHeader(std::ofstream& ofs, AssetHeader& header, AssetSerializer& serializer);

        static void writeStringTable(std::ofstream& ofs, const AssetHeader& header, AssetSerializer& serializer);

        static void writeImportTable(std::ofstream& ofs, const AssetHeader& header, AssetSerializer& serializer);

        static void writeObjectBuffers(std::ofstream& ofs, const AssetHeader& header, AssetSerializer& serializer);
    };
}
