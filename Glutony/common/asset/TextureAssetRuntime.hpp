#pragma once

#include <GL/glew.h>

#include <string>

namespace asset
{
GLuint resolveTextureAssetTextureId(const std::string& assetPath, bool allowFallback = true);
}
