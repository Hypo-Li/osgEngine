#include "Context.h"
#include "AssetManager.h"
#include <Engine/Render/Material.h>
namespace xxx
{
    Material* Context::getDefaultMaterial() const
    {
        Asset* defaultMaterialAsset = AssetManager::get().getAsset("Engine/Material/TestMaterial");
        if (defaultMaterialAsset)
            return defaultMaterialAsset->getRootObjectSafety<Material>();
        return nullptr;
    }
}
