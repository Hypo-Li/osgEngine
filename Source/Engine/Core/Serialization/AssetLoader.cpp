#include "AssetLoader.h"
#include "../Asset.h"
#include "../AssetManager.h"

namespace xxx
{
    using namespace refl;
    void AssetLoader::serializeObject(Object*& object)
    {
        Class* clazz = object->getClass();
        size_t propertyCount;
        serialize(&propertyCount);
        std::vector<Property*> properties = clazz->getProperties();
        for (size_t i = 0; i < propertyCount; ++i)
        {
            std::string propertyName;
            std::string typeName;
            serializeStdString(&propertyName);
            serializeStdString(&typeName);
            auto findResult = std::find_if(properties.begin(), properties.end(),
                [propertyName](const Property* prop)->bool
                {
                return propertyName == prop->getName();
                }
            );
            if (findResult != properties.end() && (*findResult)->getDeclaredType()->getName() == typeName)
            {
                Property* prop = *findResult;
                properties.erase(findResult);

                void* valuePtr = prop->getValuePtr(object);
                serializeType(prop->getDeclaredType(), valuePtr);
            }
            else
            {
                // skip this property
            }
        }

        object->postSerialize();
    }

    void AssetLoader::serializeClass(Class* clazz, void* data, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
        {
            Object* object = (static_cast<Object**>(data))[i];
            int32_t index;
            serialize(&index);
            if (index > 0)
            {
                // imported object
                auto [pathStringIndex, guid] = mRawImportTable[index - 1];
                const std::string& path = mRawStringTable.at(pathStringIndex);

                Asset* asset;
                asset = AssetManager::getAsset(path);
                object = asset->getRootObject();
            }
            else
            {
                // exported object
                index = -index - 1;
                if (mRawExportTable[index].second == nullptr)
                {
                    // if no loaded
                    Guid guid = mRawExportTable[index].first;
                    object = static_cast<Object*>(clazz->newInstance());
                    // object->mGuid = guid;
                    pushObjectBufferIndex(index);
                    serializeObject(object);
                    popObjectBufferIndex();
                }
                else
                {
                    // if loaded
                    object = mRawExportTable[index].second;
                }
            }
        }
    }
}
