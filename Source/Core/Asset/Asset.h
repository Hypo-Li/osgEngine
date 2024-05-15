#pragma once
#include <osg/Referenced>
#include <filesystem>
namespace xxx
{
    namespace fs = std::filesystem;
    class Asset : public osg::Referenced
    {
    public:
        Asset(const fs::path& assetPath) : _path(assetPath) { load(); }
        virtual ~Asset() = 0;

        virtual bool load() = 0;
        virtual bool save() const = 0;

    protected:
        std::string_view _path;
    };
}
