#include <common/Scene.hpp>
#include <iostream>

void sceneMain(Scene& scene)
{
	scene.addGameObject("Generated Root");
    std::cout << "Hello from scene script!" << std::endl;
}
