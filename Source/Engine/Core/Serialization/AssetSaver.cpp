#include "AssetSaver.h"
#include "../Asset.h"

namespace xxx
{
    using namespace refl;

    void AssetSaver::serializeObject(Object*& object)
    {
        object->preSerialize();

        Class* clazz = object->getClass();
        const Object* defaultObject = clazz->getDefaultObject();
        std::vector<Property*> serializedProperties;
        Class* baseClass = clazz;
        while (baseClass)
        {
            for (Property* prop : baseClass->getProperties())
            {
                if (!prop->compare(defaultObject, object))
                    serializedProperties.emplace_back(prop);
            }
            baseClass = baseClass->getBaseClass();
        }
        uint32_t propertyCount = serializedProperties.size();
        serialize(&propertyCount);

        for (Property* prop : serializedProperties)
        {
            std::string propertyName(prop->getName());
            std::string typeName(prop->getDeclaredType()->getName());
            serializeStdString(&propertyName);
            serializeStdString(&typeName);

            uint32_t propertySize = 0;
            serialize(&propertySize);
            uint32_t propertyBeginPos = tell();

            void* valuePtr = prop->getValuePtr(object);
            serializeType(prop->getDeclaredType(), valuePtr);

            uint32_t propertyEndPos = tell();
            propertySize = propertyEndPos - propertyBeginPos;
            seek(propertyBeginPos);
            serialize(&propertySize);
            seek(propertyEndPos);
        }
    }

    void AssetSaver::serializeClass(Class* clazz, void* data, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
        {
            Object* object = (static_cast<Object**>(data))[i];

            if (object->getAsset() != mAsset)
            {
                // imported object
                auto findResult = std::find_if(
                    mRawImportTable.begin(),
                    mRawImportTable.end(),
                    [object](const std::pair<uint32_t, Guid>& item)
                    {
                        return item.second == object->getGuid();
                    }
                );

                int32_t index;
                if (findResult == mRawImportTable.end())
                {
                    // if no saved
                    mRawStringTable.emplace_back(object->getAsset()->getPath());
                    mRawImportTable.emplace_back(mRawStringTable.size() - 1, object->getGuid());
                    index = mRawImportTable.size() - 1;
                }
                else
                {
                    // if saved
                    index = findResult - mRawImportTable.begin();
                }

                index = index + 1;
                serialize(&index);
            }
            else
            {
                // exported object
                auto findResult = std::find_if(
                    mRawExportTable.begin(),
                    mRawExportTable.end(),
                    [object](const std::pair<Guid, osg::ref_ptr<Object>>& item)
                    {
                        return item.first == object->getGuid();
                    }
                );

                int32_t index;
                if (findResult == mRawExportTable.end())
                {
                    // if no saved
                    mRawExportTable.emplace_back(object->getGuid(), object);
                    index = mRawExportTable.size() - 1;

                    pushObjectBufferIndex(createNewObjectBuffer());
                    serializeObject(object);
                    popObjectBufferIndex();
                }
                else
                {
                    // if saved
                    int32_t index = findResult - mRawExportTable.begin();
                }

                index = -(index + 1);
                serialize(&index);
            }
        }
    }
}
