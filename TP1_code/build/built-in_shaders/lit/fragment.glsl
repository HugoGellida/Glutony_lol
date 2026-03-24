#version 330 core

in vec3 _normals;
in vec2 _uvs;
in vec3 _colors;
uniform vec3 _mainCol = vec3(1.0f, 1.0f, 1.0f);
// Ouput data
out vec3 color;


void main(){
        color = (max((dot(normalize(vec3(1, 0.75, -0.5)), normalize(_normals)) + 1.0 * 0.5), 0.1) * _mainCol);
}
