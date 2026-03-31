#version 330 core

uniform vec3 _mainCol = vec3(1.0f, 1.0f, 1.0f);
// Ouput data
out vec3 color;


void main(){
        color = (_mainCol);
}
