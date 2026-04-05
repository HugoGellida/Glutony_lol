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

void registerComponentScript(const std::string& assetPath, ComponentScriptStartEntry startEntry, ComponentScriptUpdateEntry updateEntry)
{
	const std::string normalizedPath = asset::AssetManager::normalizeRelativePath(assetPath);
	if (normalizedPath.empty())
		return;

	componentScriptRegistry()[normalizedPath] = {startEntry, updateEntry};
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
}