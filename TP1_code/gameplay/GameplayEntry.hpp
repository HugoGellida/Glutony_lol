#pragma once

#include <string>

class Scene;

namespace gameplay
{
using SceneScriptEntry = void(*)(Scene& scene);

void registerGeneratedSceneScripts();
void bootstrap();
void registerSceneScript(const std::string& assetPath, SceneScriptEntry entry);
void runSceneScript(Scene& scene);
}