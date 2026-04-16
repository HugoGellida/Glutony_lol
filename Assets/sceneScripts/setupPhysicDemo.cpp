#include <common/Scene.hpp>
#include <iostream>

void sceneMain(Scene& scene)
{
    Material * mat = scene.resolveMaterialAsset("Assets/materials/pillars_legacy_occlusion.mat");
    Mesh * mesh = scene.resolveMeshAsset("procedural/plane");
    Mesh * cubeMesh = scene.resolveMeshAsset("built-in/mesh/cube_n.obj");
    std::cout << "Setup env" << std::endl;
    {
	    GameObject * obj = scene.addGameObject("PlaneY-");
        obj -> transform.setPosition(glm::vec3(0.0f, -5.0f, 0.0f));
        obj -> setSharedComponent(mesh);
        obj -> addComponent(new component::MeshRenderer(mesh, mat));
        obj -> addComponent(new physics::PlaneCollider(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
        obj -> addComponent(new physics::RigidBody(obj));
        physics::RigidBody* rb = obj -> getComponent<physics::RigidBody>();
        rb -> isStatic = true;
        rb -> useGravity = false;
        rb -> bounciness = 2.0f;
        rb -> RecomputeInverseMass();

    }
    {
	    GameObject * obj = scene.addGameObject("PlaneX+");
        obj -> transform.setPosition(glm::vec3(5.0f, -2.5f, 0.0f));
        obj -> transform.setRotation(glm::vec3(0.0f, 0.0f, 90.0f));
        obj -> transform.setScale(glm::vec3(0.5f, 0.5f, 1.0f));
        obj -> setSharedComponent(mesh);
        obj -> addComponent(new component::MeshRenderer(mesh, mat));
        obj -> addComponent(new physics::PlaneCollider(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
        obj -> addComponent(new physics::RigidBody(obj));
        physics::RigidBody* rb = obj -> getComponent<physics::RigidBody>();
        rb -> isStatic = true;
        rb -> useGravity = false;
        rb -> RecomputeInverseMass();
    }
    {
	    GameObject * obj = scene.addGameObject("PlaneX-");
        obj -> transform.setPosition(glm::vec3(-5.0f, -2.5f, 0.0f));
        obj -> transform.setRotation(glm::vec3(0.0f, 0.0f, -90.0f));
        obj -> transform.setScale(glm::vec3(0.5f, 0.5f, 1.0f));
        obj -> setSharedComponent(mesh);
        obj -> addComponent(new component::MeshRenderer(mesh, mat));
        obj -> addComponent(new physics::PlaneCollider(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
        obj -> addComponent(new physics::RigidBody(obj));
        physics::RigidBody* rb = obj -> getComponent<physics::RigidBody>();
        rb -> isStatic = true;
        rb -> useGravity = false;
        rb -> RecomputeInverseMass();
    }
    {
	    GameObject * obj = scene.addGameObject("PlaneZ-");
        obj -> transform.setPosition(glm::vec3(0.0f, -2.5f, -5.0f));
        obj -> transform.setRotation(glm::vec3(90.0f, 0.0f, 0.0f));
        obj -> transform.setScale(glm::vec3(1.0f, 0.5f, 0.5f));
        obj -> setSharedComponent(mesh);
        obj -> addComponent(new component::MeshRenderer(mesh, mat));
        obj -> addComponent(new physics::PlaneCollider(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
        obj -> addComponent(new physics::RigidBody(obj));
        physics::RigidBody* rb = obj -> getComponent<physics::RigidBody>();
        rb -> isStatic = true;
        rb -> useGravity = false;
        rb -> RecomputeInverseMass();
    }
    {
	    GameObject * obj = scene.addGameObject("PlaneZ+");
        obj -> transform.setPosition(glm::vec3(0.0f, -2.5f, 5.0f));
        obj -> transform.setRotation(glm::vec3(-90.0f, 0.0f, 0.0f));
        obj -> transform.setScale(glm::vec3(1.0f, 0.5f, 0.5f));
        obj -> setSharedComponent(mesh);
        obj -> addComponent(new component::MeshRenderer(mesh, mat));
        obj -> addComponent(new physics::PlaneCollider(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
        obj -> addComponent(new physics::RigidBody(obj));
        physics::RigidBody* rb = obj -> getComponent<physics::RigidBody>();
        rb -> isStatic = true;
        rb -> useGravity = false;
        rb -> RecomputeInverseMass();
    }
    std::cout << "Setup cubes" << std::endl;
    {
        for (size_t i = 0; i < 10; i++)
        {
            GameObject * obj = scene.addGameObject("Cube" + std::to_string(i));
            obj -> transform.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
            obj -> transform.setScale(glm::vec3(0.5f, 0.5f, 0.5f));
            obj -> setSharedComponent(cubeMesh);
            obj -> addComponent(new component::MeshRenderer(cubeMesh, mat));
            obj -> addComponent((new physics::BoxCollider(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(1.0f, 1.0f, 1.0f))));
            obj -> addComponent(new physics::RigidBody(obj));
            physics::RigidBody* rb = obj -> getComponent<physics::RigidBody>();
            rb -> useGravity = true;
            rb -> bounciness = 1.0f;
            rb -> mass = 0.01f;
            rb -> RecomputeInverseMass();
        }
    }
    std::cout << "Manual RP rebuild" << std::endl;
    scene.buildRenderPipelines();
}
