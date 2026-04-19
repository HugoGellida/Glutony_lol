#include "TextureAssetRuntime.hpp"

#include "AssetManager.hpp"

namespace asset
{
GLuint resolveTextureAssetTextureId(const std::string& assetPath, bool allowFallback)
{
    return AssetManager::instance().resolveTextureAssetTextureId(assetPath, allowFallback);
}
}
