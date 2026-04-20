#include <common/Scene.hpp>
#include <iostream>

void sceneMain(Scene& scene)
{
	dataStruct::Material * mat   = scene.resolveMaterialAsset("built-in/materials/lit_phong.mat");
    component::Mesh * mesh = scene.resolveMeshAsset("built-in/mesh/cube_n.obj");
    if (mat == nullptr || mesh == nullptr)
        return;
    
    for (size_t i = 0; i < 10; ++i)
    {
        GameObject * cube = scene.addGameObject("Cube " + std::to_string(i));
        cube->transform.setPosition(glm::vec3(i * 0.1f - 0.5f, 10.0f, 0.0f));
        cube->transform.setRotation(glm::vec3(0.0f, i * 15.0f, 0.0f));
        cube->transform.setScale(glm::vec3(0.1f, 0.1f, 0.1f));
        cube->addComponent(new component::MeshRenderer(mesh, mat));
        cube->addComponent(new physics::BoxCollider(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(1.0f, 1.0f, 1.0f)));
        cube->addComponent(new physics::RigidBody(cube));
        physics::RigidBody* rigidBody = cube->getComponent<physics::RigidBody>();
        rigidBody->mass = 1.0f;
        rigidBody->useGravity = true;
        rigidBody->linearDamping = 0.1f;
        rigidBody->angularDamping = 0.05f;
        rigidBody->bounciness = 0.3f;
        rigidBody->RecomputeInverseMass();
        
    }
}
