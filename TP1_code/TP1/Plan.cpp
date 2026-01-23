#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>

class Plan {
    public:
    std::vector<glm::vec3> vertices;
    std::vector<unsigned short> triangles;
    Plan(float taille, glm::vec3 centre){
        float step = taille/16;
        for (int i = 0; i < 16; i++){
            for (int j = 0; j < 16; j++){
                vertices.push_back(glm::vec3(centre.x - taille/2.f + j * step, 0, centre.z - taille/2.f + i * step));
            }
        }

        for (int i = 0; i < vertices.size() - 1; i++){
            for (int j = 0; j < vertices.size() - 1; j++){
                triangles.push_back(i * 16 + j);
                triangles.push_back(i * 16 + j + 1);
                triangles.push_back((i + 1) * 16 + j);
                triangles.push_back((i + 1) * 16 + j + 1);
                triangles.push_back((i + 1) * 16 + j);
                triangles.push_back(i * 16 + j + 1);
            }
        }
    }
};