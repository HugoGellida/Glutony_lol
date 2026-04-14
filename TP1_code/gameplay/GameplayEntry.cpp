#include "GameplayEntry.hpp"

#include <common/Scene.hpp>
#include <common/asset/AssetManager.hpp>
#include <common/gameobject/GameObject.hpp>
#include <common/gameobject/component/ScriptComponent.hpp>

#include <unordered_map>

namespace gameplay
{
namespace
{
struct ComponentScriptCallbacks
{
    ComponentScriptStartEntry start = nullptr;
    ComponentScriptUpdateEntry update = nullptr;
};

struct RenderUniformFactoryCallbacks
{
	render::UniformFactoryEntry build = nullptr;
	render::UniformFactoryIterationCountEntry iterationCount = nullptr;
	render::UniformFactoryIterationGroupEntry group = nullptr;
	render::UniformFactoryIterationGroupEntry bakedGroup = nullptr;
};

std::unordered_map<std::string, SceneScriptEntry>& sceneScriptRegistry()
{
	static std::unordered_map<std::string, SceneScriptEntry> registry;
	return registry;
}

std::unordered_map<std::string, ComponentScriptCallbacks>& componentScriptRegistry()
{
	static std::unordered_map<std::string, ComponentScriptCallbacks> registry;
	return registry;
}

std::unordered_map<std::string, RenderUniformFactoryCallbacks>& renderUniformFactoryRegistry()
{
	static std::unordered_map<std::string, RenderUniformFactoryCallbacks> registry;
	return registry;
}

ComponentScriptCallbacks* findComponentScriptCallbacks(const component::ScriptComponent& scriptComponent)
{
	const std::string normalizedPath = asset::AssetManager::normalizeRelativePath(scriptComponent.getScriptAssetPath());
	if (normalizedPath.empty())
		return nullptr;

	auto it = componentScriptRegistry().find(normalizedPath);
	return it != componentScriptRegistry().end() ? &it->second : nullptr;
}
}

void bootstrap()
{
	static bool bootstrapped = false;
	if (bootstrapped)
		return;

	registerBuiltInRenderUniformFactories();
	registerGeneratedSceneScripts();
	registerGeneratedRenderUniformFactories();
	bootstrapped = true;
}

void registerSceneScript(const std::string& assetPath, SceneScriptEntry entry)
{
	const std::string normalizedPath = asset::AssetManager::normalizeRelativePath(assetPath);
	if (normalizedPath.empty() || entry == nullptr)
		return;

	sceneScriptRegistry()[normalizedPath] = entry;
}

void registerComponentScript(const std::string& assetPath, ComponentScriptStartEntry startEntry, ComponentScriptUpdateEntry updateEntry)
{
	const std::string normalizedPath = asset::AssetManager::normalizeRelativePath(assetPath);
	if (normalizedPath.empty())
		return;

	componentScriptRegistry()[normalizedPath] = {startEntry, updateEntry};
}

void registerRenderUniformFactory(
	const std::string& assetPath,
	render::UniformFactoryEntry entry,
	render::UniformFactoryIterationCountEntry iterationCountEntry,
	render::UniformFactoryIterationGroupEntry groupEntry,
	render::UniformFactoryIterationGroupEntry bakedGroupEntry)
{
	const std::string normalizedPath = asset::AssetManager::normalizeRelativePath(assetPath);
	if (normalizedPath.empty() || entry == nullptr)
		return;

	renderUniformFactoryRegistry()[normalizedPath] = {entry, iterationCountEntry, groupEntry, bakedGroupEntry};
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

void runPendingComponentScriptStarts(Scene& scene)
{
	for (size_t gameObjectIndex = 0; gameObjectIndex < scene.getGameObjectCount(); ++gameObjectIndex)
	{
		GameObject* gameObject = scene.getGameObject(gameObjectIndex);
		if (gameObject == nullptr)
			continue;

		for (size_t componentIndex = 0; componentIndex < gameObject->getComponentCount(); ++componentIndex)
		{
			auto* scriptComponent = dynamic_cast<component::ScriptComponent*>(gameObject->getComponentAt(componentIndex));
			if (scriptComponent == nullptr || scriptComponent->hasRuntimeStarted())
				continue;

			ComponentScriptCallbacks* callbacks = findComponentScriptCallbacks(*scriptComponent);
			if (callbacks == nullptr)
				continue;

			asset::DataAssetDefinition* dataAssetDefinition = scriptComponent->loadDataAssetDefinition();
			if (callbacks->start != nullptr)
				callbacks->start(scene, *gameObject, *scriptComponent, dataAssetDefinition);

			scriptComponent->markRuntimeStarted();
		}
	}
}

void runComponentScriptUpdates(Scene& scene, float deltaTime)
{
	for (size_t gameObjectIndex = 0; gameObjectIndex < scene.getGameObjectCount(); ++gameObjectIndex)
	{
		GameObject* gameObject = scene.getGameObject(gameObjectIndex);
		if (gameObject == nullptr)
			continue;

		for (size_t componentIndex = 0; componentIndex < gameObject->getComponentCount(); ++componentIndex)
		{
			auto* scriptComponent = dynamic_cast<component::ScriptComponent*>(gameObject->getComponentAt(componentIndex));
			if (scriptComponent == nullptr || !scriptComponent->hasRuntimeStarted())
				continue;

			ComponentScriptCallbacks* callbacks = findComponentScriptCallbacks(*scriptComponent);
			if (callbacks == nullptr || callbacks->update == nullptr)
				continue;

			asset::DataAssetDefinition* dataAssetDefinition = scriptComponent->loadDataAssetDefinition();
			callbacks->update(scene, *gameObject, *scriptComponent, dataAssetDefinition, deltaTime);
		}
	}
}

bool queryRenderUniformFactoryIterationCount(const std::string& assetPath, const render::UniformFactoryExecutionContext& context, int& iterationCountOut)
{
	const std::string normalizedPath = asset::AssetManager::normalizeRelativePath(assetPath);
	if (normalizedPath.empty())
	{
		iterationCountOut = 0;
		return false;
	}

	const auto it = renderUniformFactoryRegistry().find(normalizedPath);
	if (it == renderUniformFactoryRegistry().end())
	{
		iterationCountOut = 0;
		return false;
	}

	if (it->second.iterationCount == nullptr)
	{
		iterationCountOut = 1;
		return true;
	}

	iterationCountOut = it->second.iterationCount(context);
	return true;
}

bool queryRenderUniformFactoryIterationGroup(const std::string& assetPath, const render::UniformFactoryExecutionContext& context, std::string& groupOut)
{
	groupOut.clear();

	const std::string normalizedPath = asset::AssetManager::normalizeRelativePath(assetPath);
	if (normalizedPath.empty())
		return false;

	const auto it = renderUniformFactoryRegistry().find(normalizedPath);
	if (it == renderUniformFactoryRegistry().end())
		return false;

	if (it->second.group == nullptr)
		return true;

	groupOut = it->second.group(context);
	return true;
}

bool queryRenderUniformFactoryBakedIterationGroup(const std::string& assetPath, const render::UniformFactoryExecutionContext& context, std::string& groupOut)
{
	groupOut.clear();

	const std::string normalizedPath = asset::AssetManager::normalizeRelativePath(assetPath);
	if (normalizedPath.empty())
		return false;

	const auto it = renderUniformFactoryRegistry().find(normalizedPath);
	if (it == renderUniformFactoryRegistry().end())
		return false;

	if (it->second.bakedGroup == nullptr)
		return true;

	groupOut = it->second.bakedGroup(context);
	return true;
}

bool runRenderUniformFactory(const std::string& assetPath, const render::UniformFactoryExecutionContext& context, const render::UniformFactoryWriter& writer)
{
	const std::string normalizedPath = asset::AssetManager::normalizeRelativePath(assetPath);
	if (normalizedPath.empty())
		return false;

	const auto it = renderUniformFactoryRegistry().find(normalizedPath);
	if (it == renderUniformFactoryRegistry().end() || it->second.build == nullptr)
		return false;

	return it->second.build(context, writer);
}
}