#version 330 core

in vec3 _normals;
in vec2 _uvs;
in vec3 _colors;
uniform vec3 _mainCol = vec3(1.0f, 1.0f, 1.0f);
// Ouput data
out vec3 color;


void main(){
        color = (dot(normalize(vec3(-1, -1, 0)), normalize(_normals)) * _mainCol);
}
