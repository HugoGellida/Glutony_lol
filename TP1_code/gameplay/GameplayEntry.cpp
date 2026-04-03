#include "GameplayEntry.hpp"

#include <common/Scene.hpp>
#include <common/asset/AssetManager.hpp>

#include <unordered_map>

namespace gameplay
{
namespace
{
std::unordered_map<std::string, SceneScriptEntry>& sceneScriptRegistry()
{
	static std::unordered_map<std::string, SceneScriptEntry> registry;
	return registry;
}
}

void bootstrap()
{
	static bool bootstrapped = false;
	if (bootstrapped)
		return;

	registerGeneratedSceneScripts();
	bootstrapped = true;
}

void registerSceneScript(const std::string& assetPath, SceneScriptEntry entry)
{
	const std::string normalizedPath = asset::AssetManager::normalizeRelativePath(assetPath);
	if (normalizedPath.empty() || entry == nullptr)
		return;

	sceneScriptRegistry()[normalizedPath] = entry;
}

void runSceneScript(Scene& scene)
{
	const std::string normalizedPath = asset::AssetManager::normalizeRelativePath(scene.getSceneScriptAssetPath());
	if (normalizedPath.empty())
		return;

	const auto it = sceneScriptRegistry().find(normalizedPath);
	if (it == sceneScriptRegistry().end() || it->second == nullptr)
		return;

	it->second(scene);
}
}